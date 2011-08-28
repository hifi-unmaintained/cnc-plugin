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
#include "updater.h"
#include "http.h"
#include "parse.h"

#include <string.h>
#include <stdlib.h>

updater_index *updater_index_parse(char *buf)
{
    int num_files = parse_line_count(buf) - 1;
    char *line = parse_line(buf);
    updater_index *dl_idx;

    if (line == NULL)
    {
        return NULL;
    }

    dl_idx = calloc(1, sizeof(updater_index));
    memset(dl_idx, 0, sizeof(updater_index));
    dl_idx->version = atoi(line);
    dl_idx->num_files = num_files;
    dl_idx->files = num_files > 0 ? calloc(1, sizeof(updater_index_file *) * dl_idx->num_files) : NULL;

    for (int i = 0; i < dl_idx->num_files; i++)
    {
        dl_idx->files[i] = parse_hash(parse_line(NULL));
    }

    return dl_idx;
}

void updater_index_free(updater_index *index)
{
    if (index->files)
    {
        for (int i = 0; i < index->num_files; i++)
        {
            free(index->files[i]);
        }

        free(index->files);
    }

    free(index);
}

int UpdaterThread(InstanceData *data)
{
    int version = 0;
    updater_index *index;
    char *buf = calloc(1, MEMSIZ);

    FILE *fh = fopen(parse_url_get_file(data->url), "r");
    if (fh)
    {
        fread(buf, MEMSIZ-1, 1, fh);
        fclose(fh);

        index = updater_index_parse(buf);
        version = index->version;
        updater_index_free(index);
    }

    if (http_download_mem(data, data->url, buf, MEMSIZ-1) == CURLE_OK)
    {
        char *tmp = calloc(1, MEMSIZ);
        strncpy(tmp, buf, MEMSIZ);
        index = updater_index_parse(tmp);
        free(tmp);

        printf("Our version: %d\nTheir version: %d\n", version, index->version);

        if (version < index->version)
        {
            for (int i = 0; i < index->num_files; i++)
            {
                if (index->files[i])
                {
                    if (http_download(data, parse_url_set_file(data->url, index->files[i]->name)) != CURLE_OK)
                    {
                        SetStatus(data, "Couldn't download %s :-(", index->files[i]->name);
                        free(buf);
                        updater_index_free(index);
                        return 0;
                    }
                }
            }

            fh = fopen(parse_url_get_file(data->url), "w");
            if (fh)
            {
                fwrite(buf, strlen(buf), 1, fh);
                fclose(fh);
            }
        }

        updater_index_free(index);
        free(buf);
        return LauncherThread(data);
    }
    else
    {
        SetStatus(data, "Couldn't download %s :-(", parse_url_get_file(data->url));
    }

    free(buf);
    return 0;
}
