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

#include "gzip.h"
#include <zlib.h>
#include <stdio.h>
#include <errno.h>

bool gzip_uncompress_file(InstanceData *data, const char *src, const char *dst)
{
    gzFile *gz_fh;
    FILE *fh;
    char buf[128];
    int ret;

    SetStatus(data, "Uncompressing %s...", dst);

    gz_fh = gzopen(src, "rb");
    if (!gz_fh)
    {
        return false;
    }

    fh = fopen(dst, "wb");
    if (!fh)
    {
        gzclose(gz_fh);
        return false;
    }

    do
    {
        ret = gzread(gz_fh, buf, sizeof(buf));
        if (ret > 0)
        {
            if (fwrite(buf, ret, 1, fh) < 1)
            {
                ret = -1;
                break;
            }
        }
    } while (ret > 0);

    gzclose(gz_fh);
    fclose(fh);

    return (ret == 0);
}
