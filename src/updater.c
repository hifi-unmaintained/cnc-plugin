/*
 * Copyright (c) 2011 Toni Spets <toni.spets@iki.fi>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "plugin.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <curl/curl.h>
#include <zlib.h>

#define MEMSIZ 4096

typedef struct {
    InstanceData *data;
    CURL *curl;
} download;

typedef struct {
    download *dl;
    const char *file;
    void *buf;
    size_t bufsiz;
    size_t bufpos;
} download_mem;

typedef struct {
    download *dl;
    const char *file;
    FILE *fh;
} download_disk;

typedef struct {
    char str_hash[33];
    char hash[16];
    char file[64];
} download_index_file;

typedef struct {
    unsigned int version;
    int num_files;
    download_index_file **files;
} download_index;

const char *updater_get_url(download *dl, const char *file)
{
    static char buf[2048];
    char *ptr;
    strncpy(buf, dl->data->url, sizeof(buf)-1);

    ptr = strrchr(buf, '/');
    if (ptr && strlen(ptr) > 1) {
        *(++ptr) = '\0';
        strncat(buf, file, sizeof(buf)-1);
        return buf;
    }

    return NULL;
}

const char *updater_get_file(const char *url)
{
    static char buf[2048];
    char *ptr = strrchr(url, '/');

    if (ptr && strlen(ptr) > 1) {
        strncpy(buf, ++ptr, sizeof(buf)-1);
        return buf;
    }

    return NULL;
}

size_t updater_write_mem(void *buf, size_t len, size_t nmemb, download_mem *dl_mem)
{
    if (dl_mem->bufpos + (len * nmemb) > dl_mem->bufsiz)
    {
        return 0;
    }

    memcpy((char *)dl_mem->buf + dl_mem->bufpos, buf, len * nmemb);

    dl_mem->bufpos += len * nmemb;
    return len * nmemb;
}

CURLcode updater_download_mem(download *dl, const char *url, void *buf, size_t bufsiz)
{
    static download_mem dl_mem;
    dl_mem.dl = dl;
    dl_mem.file = updater_get_file(url);
    dl_mem.buf = buf;
    dl_mem.bufsiz = bufsiz;
    dl_mem.bufpos = 0;

    curl_easy_setopt(dl->curl, CURLOPT_URL, url);
    curl_easy_setopt(dl->curl, CURLOPT_WRITEFUNCTION, updater_write_mem);
    curl_easy_setopt(dl->curl, CURLOPT_WRITEDATA, &dl_mem);
    curl_easy_setopt(dl->curl, CURLOPT_FAILONERROR, true);

    return curl_easy_perform(dl->curl);
}

size_t updater_write_disk(void *buf, size_t len, size_t nmemb, download_disk *dl_disk)
{
    return fwrite(buf, len, nmemb, dl_disk->fh) * len;
}

CURLcode updater_download_disk(download *dl, const char *url, FILE *fh)
{
    static download_disk dl_disk;
    dl_disk.dl = dl;
    dl_disk.file = updater_get_file(url);
    dl_disk.fh = fh;

    curl_easy_setopt(dl->curl, CURLOPT_URL, url);
    curl_easy_setopt(dl->curl, CURLOPT_WRITEFUNCTION, updater_write_disk);
    curl_easy_setopt(dl->curl, CURLOPT_WRITEDATA, &dl_disk);
    curl_easy_setopt(dl->curl, CURLOPT_FAILONERROR, true);

    return curl_easy_perform(dl->curl);
}

char *updater_parse_line(char *buf)
{
    static char *p = NULL;
    static char *last_buf = NULL;
    char *line;

    if (buf != last_buf)
    {
        p = buf;
        last_buf = buf;
    }

    if (buf == NULL)
    {
        return NULL;
    }

    line = p;

    while (*p != '\0' && *p != '\n')
    {
        p++;
    }

    if (p > buf && *(p-1) == '\r')
    {
        *(p-1) = '\0';
    }

    if (*p != '\0')
    {
        *p = '\0';
        p++;
    }

    return (*line != '\0' ? line : NULL);
}

int updater_count_lines(const char *buf)
{
    int ret = 0;
    char *p = (char *)buf;

    while (*p != '\0')
    {
        if (*p == '\n')
        {
            ret++;
        }

        p++; 
    }

    return ret;
}

download_index_file *updater_parse_hash(char *buf)
{
    char *hash = buf;
    char *file;
    char *p = buf;
    download_index_file *dl_idx_file;

    while (*p != '\0' && *p != '\t' && *p != ' ')
    {
        p++;
    }

    if (*p == '\0')
    {
        return NULL;
    }

    *p++ = '\0';

    while (*p == ' ' || *p == '\t')
    {
        p++;
    }

    if (*p == '\0')
    {
        return NULL;
    }

    file = p;
    p += strlen(p) - 1;

    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
    {
        *p = '\0';
        p--;
    }

    dl_idx_file = calloc(1, sizeof(download_index_file));

    strncpy(dl_idx_file->str_hash, hash, sizeof(dl_idx_file->str_hash)-1);
    strncpy(dl_idx_file->file, file, sizeof(dl_idx_file->file)-1);

    return dl_idx_file;
}

download_index *updater_parse_index(char *buf)
{
    updater_parse_line(NULL); // hack to reset buffer, should change to strtok style

    int num_files = updater_count_lines(buf) - 1;
    char *line = updater_parse_line(buf);
    download_index *dl_idx;

    if (line == NULL)
    {
        return NULL;
    }

    dl_idx = calloc(1, sizeof(download_index));
    memset(dl_idx, 0, sizeof(download_index));
    dl_idx->version = atoi(line);
    dl_idx->num_files = num_files;
    dl_idx->files = num_files > 0 ? calloc(1, sizeof(download_index_file *) * dl_idx->num_files) : NULL;

    for (int i = 0; i < dl_idx->num_files; i++)
    {
        dl_idx->files[i] = updater_parse_hash(updater_parse_line(buf));
    }

    return dl_idx;
}

bool updater_handle_download(download *dl, download_index_file *file)
{
    char gz_path[512];
    char gz_url[512];
    char gz_buf[512];
    CURLcode ret;
    size_t len;

    snprintf(gz_path, sizeof(gz_path), "%s.gz", file->file);
    snprintf(gz_url, sizeof(gz_url), "%s.gz", updater_get_url(dl, file->file));

    FILE *fh = fopen(gz_path, "wb");
    if (fh)
    {
        ret = updater_download_disk(dl, gz_url, fh);

        if (ret == CURLE_OK)
        {
            fclose(fh);

            gzFile gz = gzopen(gz_path, "rb");

            if (gz)
            {
                fh = fopen(file->file, "wb");
                if (fh)
                {
                    while ((len = gzread(gz, gz_buf, sizeof(gz_buf))) > 0)
                    {
                        fwrite(gz_buf, len, 1, fh);
                    }
                    fclose(fh);
                    gzclose(gz);
                    unlink(gz_path);
                    return true;
                }
                return false;
            }

            return false; /* invalid gz file */
        }

        rewind(fh);
        ret = updater_download_disk(dl, file->file, fh);
        fclose(fh);

        if (ret == CURLE_OK)
        {
            return true;
        }
    }

    return false;
}

int UpdaterThread(InstanceData *data)
{
    download dl;
    download_index *dl_idx;
    download_index *loc_idx;
    CURLcode ret;
    FILE *fh;

    dl.data = data;
    dl.curl = curl_easy_init();

    if (dl.curl)
    {
        char *buf = calloc(1, MEMSIZ);
        char *buf2 = calloc(1, MEMSIZ);
        ret = updater_download_mem(&dl, dl.data->url, buf, MEMSIZ-1);

        if (ret == CURLE_OK)
        {
            memcpy(buf2, buf, strlen(buf));

            SetStatus(dl.data, "Checking version...");
            dl_idx = updater_parse_index(buf);
            printf("Latest version: %d\n", dl_idx->version);

            fh = fopen(updater_get_file(dl.data->url), "rb");
            if (fh)
            {
                fread(buf, MEMSIZ, 1, fh);
                fclose(fh);

                loc_idx = updater_parse_index(buf);
                if (loc_idx)
                {
                    printf("Current version: %d\n", loc_idx->version);
                    if (loc_idx->version >= dl_idx->version)
                    {
                        printf("Nothing to update.\n");
                        LauncherThread(data);
                        return 0;
                    }
                }
                free(loc_idx);
            }
            else
            {
                printf("Current version: n/a\n");
            }

            for (int i = 0; i < dl_idx->num_files; i++)
            {
                SetStatus(dl.data, "Downloading %s...", dl_idx->files[i]->file);
                if (!updater_handle_download(&dl, dl_idx->files[i]))
                {
                    SetStatus(data, "FFFUUU! Failed to download %s.", dl_idx->files[i]->file);
                    return 0;
                }
            }

            fh = fopen(updater_get_file(dl.data->url), "wb");
            fwrite(buf2, strlen(buf2), 1, fh);
            fclose(fh);
        }
        else
        {
            int http_code = 0;
            curl_easy_getinfo(dl.curl, CURLINFO_RESPONSE_CODE, &http_code);
            SetStatus(dl.data, "Error %d when fetching version information :-(", http_code);
        }

        curl_easy_cleanup(dl.curl);
    }
    else
    {
        SetStatus(data, "Oh noes! Couldn't initialize libcURL! :-(");
    }

    LauncherThread(data);
    return 0;
}
