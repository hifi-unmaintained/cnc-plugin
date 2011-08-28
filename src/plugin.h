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

/* nptypes.h expect VC and do not test for gcc so these are required */
#include <stdbool.h>
#include <stdint.h>

#include <npapi.h>
#include <npfunctions.h>

#include <windows.h>

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

typedef struct InstanceData
{
    NPP npp;
    NPWindow window;
    HWND hWnd;
    HANDLE hProcess;
    HANDLE hThread;
    char status[256];

    /* app related */
    char application[256];
    char executable[64];
    char url[MAX_PATH];
    char path[MAX_PATH];
    char config[MAX_PATH];
} InstanceData;

int SetStatus(InstanceData *data, const char *fmt, ...);
int UpdaterThread(InstanceData *data);
int LauncherThread(InstanceData *data);

extern NPP is_running;

#endif
