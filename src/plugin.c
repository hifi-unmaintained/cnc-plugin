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
#include "strl.h"

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

static NPNetscapeFuncs* browser = NULL;

static char plugin_name[MAX_PATH] = { 0 };
static char plugin_path[MAX_PATH] = { 0 };

static HINSTANCE plugin_hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        plugin_hInstance = hinstDLL;
    }

    return TRUE;
}

#ifdef _DEBUG
BOOL con = FALSE;
#endif

NPP is_running = NULL;

BOOL FileExists(const char *path)
{
    FILE* file;

    if (path == NULL || strlen(path) == 0)
    {
        return FALSE;
    }

    if( (file = fopen(path, "r")) )
    {
        fclose(file);
        return TRUE;
    }
    return FALSE;
}

int SetStatus(InstanceData *data, const char *fmt, ...)
{
    va_list args;
    RECT rc = { 0, 0, data->window.width, data->window.height };

    if (fmt == NULL)
    {
        return 0;
    }

    va_start(args, fmt);
    vsnprintf(data->status, sizeof(data->status), fmt, args);
    va_end(args);

    InvalidateRect(data->window.window, &rc, TRUE);

    return 1;
}

NPError WINAPI NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
    printf("NP_GetEntryPoints(pFuncs=%p)\n", pFuncs);

    pFuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
    pFuncs->size          = sizeof(NPPluginFuncs);

    pFuncs->newp          = NPP_New;
    pFuncs->destroy       = NPP_Destroy;
    pFuncs->setwindow     = NPP_SetWindow;
    pFuncs->newstream     = NPP_NewStream;
    pFuncs->destroystream = NPP_DestroyStream;
    pFuncs->asfile        = NPP_StreamAsFile;
    pFuncs->writeready    = NPP_WriteReady;
    pFuncs->write         = NPP_Write;
    pFuncs->print         = NPP_Print;
    pFuncs->event         = NPP_HandleEvent;
    pFuncs->urlnotify     = NPP_URLNotify;
    pFuncs->getvalue      = NPP_GetValue;
    pFuncs->setvalue      = NPP_SetValue;
    pFuncs->javaClass     = NULL;

    return NPERR_NO_ERROR;
}

NPError WINAPI NP_Initialize(NPNetscapeFuncs* bFuncs)
{
#ifdef _DEBUG
    if (!con)
    {
        con = AllocConsole();
        freopen("CONOUT$", "wt", stdout);
        SetConsoleTitle("C&C Plugin Debug Console");
    }
#endif

    printf("NP_Initialize(bFuncs=%p)\n", bFuncs);

    browser = bFuncs;

    if (GetModuleFileNameA(plugin_hInstance, plugin_path, sizeof(plugin_path)) > 0)
    {
        char *ptr = strrchr(plugin_path, '\\');
        if (ptr)
        {
            *ptr = 0;
            strlcpy(plugin_name, ptr+1, sizeof(plugin_name));
            ptr = strrchr(plugin_name, '.');
            if (ptr) *ptr = '\0';
        }
    }

    return NPERR_NO_ERROR;
}


NPError OSCALL NP_Shutdown()
{
    printf("NP_Shutdown()\n");
#ifdef _DEBUG
    if (con)
    {
        /* keep our console open */
        while (1)
        {
            Sleep(1000);
        }
    }
#endif

    return NPERR_NO_ERROR;
}

NPError NP_GetValue(void* future, NPPVariable aVariable, void* aValue)
{
    printf("NP_GetValue(future=%p, aVariable=%d, aValue=%p)\n", future, aVariable, aValue);
    return NPERR_INVALID_PARAM;
}

NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData* saved)
{
    printf("NPP_New(pluginType=%s, instance=%p, ...)\n", pluginType, instance);
    browser->setvalue(instance, NPPVpluginWindowBool, (void*)TRUE);

    InstanceData* data = (InstanceData*)malloc(sizeof(InstanceData));
    if (!data)
        return NPERR_OUT_OF_MEMORY_ERROR;

    memset(data, 0, sizeof(InstanceData));
    data->npp = instance;
    instance->pdata = data;

    if (is_running && is_running != instance)
    {
        SetStatus(data, "A game is already running.");
        return NPERR_NO_ERROR;
    }

    is_running = instance;

    printf(" params     : %d\n", argc);
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argn[i], "game") == 0)
        {
            const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
            int j;
            for (j = 0; j < strlen(argv[i]); j++)
            {
                if (strchr(charset, argv[i][j]) == NULL)
                {
                    break;
                }
            }

            if (j == strlen(argv[i]))
            {
                snprintf(data->path, sizeof(data->path), "%s\\%s", plugin_path, argv[i]);
                snprintf(data->config, sizeof(data->config), "%s\\%s.ini", plugin_path, plugin_name);
                strlcpy(data->game, argv[i], sizeof(data->game));

                printf(" game       : %s\n", data->game);
                printf(" path       : %s\n", data->path);
                printf(" config     : %s\n", data->config);
            }
            break;
        }
    }

    if (!FileExists(data->config))
    {
        SetStatus(data, "Oh noes, can't find configuration file! :-(");
        is_running = NULL;
        return NPERR_NO_ERROR;
    }

    GetPrivateProfileStringA(data->game, "application", NULL, data->application, sizeof(data->application), data->config);
    GetPrivateProfileStringA(data->game, "executable", NULL, data->executable, sizeof(data->executable), data->config);
    GetPrivateProfileStringA(data->game, "url", NULL, data->url, sizeof(data->url), data->config);
    printf(" application: %s\n", data->application);
    printf(" executable : %s\n", data->executable);
    printf(" url        : %s\n", data->url);

    mkdir(data->path);
    if (chdir(data->path) < 0)
    {
        SetStatus(data, "Failed to create data directory :-(");
        return NPERR_NO_ERROR;
    }


    if (strlen(data->application) == 0 || strlen(data->executable) == 0 || strlen(data->url) == 0)
    {
        SetStatus(data, "Oh noes, the configuration file is corrupted! :-(");
    }
    else
    {
        data->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UpdaterThread, (void *)data, 0, NULL);
    }

    return NPERR_NO_ERROR;
}

NPError NPP_Destroy(NPP instance, NPSavedData** save)
{
    printf("NPP_Destroy(instance=%p, save=%p)\n", instance, save);
    InstanceData* data = (InstanceData*)(instance->pdata);

    if (data->hProcess)
    {
        TerminateProcess(data->hProcess, 0);
        is_running = NULL;
    }

    free(data);
    return NPERR_NO_ERROR;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    InstanceData* data = (InstanceData*)GetWindowLong(hWnd, GWL_USERDATA);
    static BOOL locked = FALSE;
    POINT pt = { 0, 0 };
    RECT rc = { 0, 0, data->window.width, data->window.height };

    switch (uMsg)
    {
        case WM_USER:
            data->hWnd = (HWND)lParam;
            if (data->hWnd == NULL)
            {
                SetStatus(data, "GAME OVER");
                if (locked)
                {
                    ClipCursor(NULL);
                    while(ShowCursor(TRUE) < 0);
                    locked = FALSE;
                }
            }
            return TRUE;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            if (!locked && data->hWnd)
            {
                ClientToScreen(hWnd, &pt);
                rc.left = pt.x;
                rc.top = pt.y;
                rc.right = rc.left + data->window.width;
                rc.bottom = rc.top + data->window.height;
                ClipCursor(&rc);

                while(ShowCursor(FALSE) >= 0);
                locked = TRUE;

                PostMessage(data->hWnd, WM_MOUSEMOVE, wParam, lParam);

                return TRUE;
            }
        case WM_KEYDOWN:
            if (locked && (wParam == VK_CONTROL || wParam == VK_TAB))
            {
                if(GetAsyncKeyState(VK_CONTROL) & 0x8000 && GetAsyncKeyState(VK_TAB) & 0x8000)
                {
                    ClipCursor(NULL);
                    while(ShowCursor(TRUE) < 0);
                    locked = FALSE;
                }
            }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MOUSEMOVE:
        case WM_KEYUP:
            if (!locked)
            {
                return TRUE;
            }
        case WM_ERASEBKGND:
            return PostMessage(data->hWnd, uMsg, wParam, lParam);
        case WM_PAINT:
            if (data->hWnd == NULL)
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                RECT rc = { 0, 0, data->window.width, data->window.height };
                SetBkColor(hdc, RGB(0,0,0));
                SetTextColor(hdc, RGB(255,255,255));
                FillRect(hdc, &rc, (HBRUSH) GetStockObject(BLACK_BRUSH));
                DrawText(hdc, data->status, -1, &rc, DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_CENTER);
                EndPaint(hWnd, &ps);
                return TRUE;
            }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

NPError NPP_SetWindow(NPP instance, NPWindow* window)
{
    InstanceData* data = (InstanceData*)(instance->pdata);

    printf("NPP_SetWindow(instance=%p, window=%p)\n", instance, window);

    data->window = *window;

    SetWindowLong(window->window, GWL_WNDPROC, (LONG)WndProc);
    SetWindowLong(window->window, GWL_USERDATA, (LONG)data);

    /* give out our new window in case it actually changed, usually it does not change */
    if (data->hWnd)
    {
        PostMessage(data->hWnd, WM_USER, 0, (LPARAM)data->window.window);
    }

    return NPERR_NO_ERROR;
}

NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype)
{
    printf("NPP_NewStream(instance=%p, ...)\n", instance);
    return NPERR_NO_ERROR;
}

NPError NPP_DestroyStream(NPP instance, NPStream* stream, NPReason reason)
{
    printf("NPP_DestroyStream(instance=%p, ...)\n", instance);
    return NPERR_NO_ERROR;
}

int32_t NPP_WriteReady(NPP instance, NPStream* stream)
{
    printf("NPP_WriteReady(instance=%p, ...)\n", instance);
    return 0;
}

int32_t NPP_Write(NPP instance, NPStream* stream, int32_t offset, int32_t len, void* buffer)
{
    printf("NPP_WriteReady(instance=%p, ...)\n", instance);
    return 0;
}

void NPP_StreamAsFile(NPP instance, NPStream* stream, const char* fname)
{
    printf("NPP_StreamAsFile(instance=%p, ...)\n", instance);
}

void NPP_Print(NPP instance, NPPrint* platformPrint)
{
    printf("NPP_Print(instance=%p, ...)\n", instance);
}

int16_t NPP_HandleEvent(NPP instance, void *event)
{
    printf("NPP_HandleEvent(instance=%p, ...)\n", instance);
    return 0;
}

void NPP_URLNotify(NPP instance, const char* URL, NPReason reason, void* notifyData)
{
    printf("NPP_URLNotify(instance=%p, ...)\n", instance);
}

NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
    printf("NPP_GetValue(instance=%p, variable=%d, value=%p)\n", instance, variable, value);
    return NPERR_GENERIC_ERROR;
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
    printf("NPP_SetValue(instance=%p, variable=%d, value=%p)\n", instance, variable, value);
    return NPERR_GENERIC_ERROR;
}
