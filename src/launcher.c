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

#include <stdio.h>
#include "plugin.h"

int LauncherThread(InstanceData *data)
{
    PROCESS_INFORMATION pInfo;
    STARTUPINFOA sInfo;

    memset(&pInfo, 0, sizeof(PROCESS_INFORMATION));
    memset(&sInfo, 0, sizeof(STARTUPINFOA));

    SetStatus(data, "Starting game...");

    SetCurrentDirectoryA(data->path);

    char buf[256];
    snprintf(buf, 256, "%d", (unsigned int)data->window.window);
    SetEnvironmentVariableA("DDRAW_WINDOW", buf);

    // FIXME: find a way to define the game and it's path
    if (CreateProcessA(data->executable, NULL, 0, 0, FALSE, 0, 0, 0, &sInfo, &pInfo))
    {
        SetStatus(data, "%s is starting...", data->application);
        data->hProcess = pInfo.hProcess;
    } else {
        SetStatus(data, "Failed to start the game.");
        is_running = NULL;
    }

    return TRUE;
}
