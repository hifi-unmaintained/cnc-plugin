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

#ifndef _HTTP_H_
#define _HTTP_H_

#include "plugin.h"
#include <curl/curl.h>

typedef struct {
    InstanceData *data;
    char name[MAX_PATH];
} download;

typedef struct {
    void *buf;
    size_t bufsiz;
    size_t bufpos;
} download_mem;

size_t http_write_mem(void *buf, size_t len, size_t nmemb, download_mem *dl_mem);
CURLcode http_download_mem(InstanceData *data, const char *url, void *buf, size_t bufsiz);
size_t http_write_file(void *buf, size_t len, size_t nmemb, FILE *fh);
CURLcode http_download_file(InstanceData *data, const char *url);
CURLcode http_download(InstanceData *data, const char *url);

#endif
