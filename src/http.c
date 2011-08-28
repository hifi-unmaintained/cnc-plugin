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

#include "http.h"
#include "parse.h"
#include "strl.h"
#include "gzip.h"
#include <unistd.h>

int http_progress(download *file, double dltotal, double dlnow, double ultotal, double ulnow)
{
    SetStatus(file->data, "Downloading %s (%d/%d) %d%%", file->name, (int)dlnow, (int)dltotal, (int)((dlnow / dltotal) * 100));
    return 0;
}

size_t http_write_mem(void *buf, size_t len, size_t nmemb, download_mem *mem)
{
    if (mem->bufpos + (len * nmemb) > mem->bufsiz)
    {
        return 0;
    }

    memcpy((char *)mem->buf + mem->bufpos, buf, len * nmemb);
    mem->bufpos += len * nmemb;

    return len * nmemb;
}

CURLcode http_download_mem(InstanceData *data, const char *url, void *buf, size_t bufsiz)
{
    CURL *curl;
    CURLcode ret;
    static download_mem mem;
    static download file;

    file.data = data;
    strlcpy(file.name, parse_url_get_file(url), sizeof(file.name));

    mem.buf = buf;
    mem.bufsiz = bufsiz;
    mem.bufpos = 0;

    curl = curl_easy_init();

    if (!curl)
    {
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, http_progress);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &file);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_mem);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);

    ret = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    
    return ret;
}

size_t http_write_file(void *buf, size_t len, size_t nmemb, FILE *fh)
{
    return fwrite(buf, len, nmemb, fh) * len;
}

CURLcode http_download_file(InstanceData *data, const char *url)
{
    CURL *curl;
    CURLcode ret;
    static download file;
    file.data = data;
    strlcpy(file.name, parse_url_get_file(url), sizeof(file.name));

    FILE *fh = fopen(file.name, "wb");
    if (!fh)
    {
        return CURLE_FAILED_INIT;
    }

    curl = curl_easy_init();

    if (!curl)
    {
        fclose(fh);
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, http_progress);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &file);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fh);

    ret = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    fclose(fh);
    
    return ret;
}

CURLcode http_download(InstanceData *data, const char *url)
{
    static char url_gz[512];

    strlcpy(url_gz, url, sizeof(url_gz));
    if (strlcat(url_gz, ".gz", sizeof(url_gz)) == sizeof(url_gz)-1)
    {
        return -1;
    }

    if (http_download_file(data, url_gz) == CURLE_OK)
    {
        strlcpy(url_gz, parse_url_get_file(url_gz), sizeof(url_gz));
        if (gzip_uncompress_file(data, url_gz, parse_url_get_file(url)))
        {
            unlink(url_gz);
            return CURLE_OK;
        }
    }
    else
    {
        return http_download_file(data, url);
    }

    return -1;
}
