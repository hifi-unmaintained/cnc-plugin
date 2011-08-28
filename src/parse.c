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
#include "parse.h"
#include "strl.h"

const char *parse_url_set_file(const char *url, const char *file)
{
    static char buf[2048];
    char *ptr;
    strlcpy(buf, url, sizeof(buf));

    ptr = strrchr(buf, '/');
    if (ptr && strlen(ptr) > 1) {
        *(++ptr) = '\0';
        strlcat(buf, file, sizeof(buf));
        return buf;
    }

    return NULL;
}

const char *parse_url_get_file(const char *url)
{
    static char buf[2048];
    char *ptr = strrchr(url, '/');

    if (ptr && strlen(ptr) > 1) {
        strlcpy(buf, ++ptr, sizeof(buf));
        return buf;
    }

    return NULL;
}

char *parse_line(char *str)
{
    static char *saveptr;

    if (str)
    {
        saveptr = str;
    }

    return parse_line_r(str, &saveptr);
}

char *parse_line_r(char *str, char **saveptr)
{
    char *p = *saveptr;
    char *line;

    if (str)
    {
        p = str;
    }

    line = p;

    while (*p != '\0' && *p != '\n')
    {
        p++;
    }

    if (p > str && *(p-1) == '\r')
    {
        *(p-1) = '\0';
    }

    if (*p != '\0')
    {
        *p = '\0';
        p++;
    }

    *saveptr = p;

    return (*line != '\0' ? line : NULL);
}

int parse_line_count(const char *buf)
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

updater_index_file *parse_hash(char *buf)
{
    char *hash = buf;
    char *name;
    char *p = buf;
    updater_index_file *dl_idx_file;

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

    name = p;
    p += strlen(p) - 1;

    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
    {
        *p = '\0';
        p--;
    }

    dl_idx_file = calloc(1, sizeof(updater_index_file));

    strlcpy(dl_idx_file->str_hash, hash, sizeof(dl_idx_file->str_hash));
    strlcpy(dl_idx_file->name, name, sizeof(dl_idx_file->name));

    return dl_idx_file;
}

bool is_zero_hash(updater_index_file *file)
{
    for (int i = 0; i < strlen(file->str_hash); i++)
    {
        if (file->str_hash[i] != '0')
        {
            return false;
        }
    }

    return true;
}
