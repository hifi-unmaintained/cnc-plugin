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

#include <windows.h>
#include <stdio.h>

/* nptypes.h expect VC and do not test for gcc so these are required */
#include <stdbool.h>
#include <stdint.h>

#include <npapi.h>
#include <npfunctions.h>

static NPNetscapeFuncs* browser = NULL;

typedef struct InstanceData
{
    NPP npp;
    NPWindow window;
    HWND hWnd;
    HANDLE hProcess;
    HANDLE hThread;
} InstanceData;

#ifdef _DEBUG
BOOL con = FALSE;
#endif

static BOOL is_running = FALSE;

static void mem_write_byte(HANDLE hProcess, DWORD address, BYTE val)
{
    DWORD dwWritten;
    VirtualProtectEx(hProcess, (void *)address, sizeof(BYTE), PAGE_EXECUTE_READWRITE, NULL);
    WriteProcessMemory(hProcess, (void *)address, &val, sizeof(BYTE), &dwWritten);
}

static void mem_write_dword(HANDLE hProcess, DWORD address, DWORD val)
{
    DWORD dwWritten;
    VirtualProtectEx(hProcess, (void *)address, sizeof(DWORD), PAGE_EXECUTE_READWRITE, NULL);
    WriteProcessMemory(hProcess, (void *)address, &val, sizeof(DWORD), &dwWritten);
}

static void mem_write_nop(HANDLE hProcess, DWORD address, DWORD len)
{
    DWORD dwWritten;
    void *data = malloc(len);
    memset(data, 0x90, len);
    VirtualProtectEx(hProcess, (void *)address, len, PAGE_EXECUTE_READWRITE, NULL);
    WriteProcessMemory(hProcess, (void *)address, data, len, &dwWritten);
    free(data);
}

void SetStatus(InstanceData *data, char *status)
{
    static char last[256];
    RECT rc = { 0, 0, data->window.width, data->window.height };
    HDC hdc = GetDC(data->window.window);

    if (status != NULL)
    {
        strncpy(last, status, 255);
    }

    SetBkColor(hdc, RGB(0,0,0));
    SetTextColor(hdc, RGB(255,255,255));
    FillRect(hdc, &rc, (HBRUSH) GetStockObject(BLACK_BRUSH));
    DrawText(hdc, last, -1, &rc, DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_CENTER);
    ReleaseDC(data->window.window, hdc);
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

    return NPERR_NO_ERROR;
}

NPError NPP_Destroy(NPP instance, NPSavedData** save)
{
    printf("NPP_Destroy(instance=%p, save=%p)\n", instance, save);
    InstanceData* data = (InstanceData*)(instance->pdata);

    if (data->hProcess)
    {
        TerminateProcess(data->hProcess, 0);
        is_running = FALSE;
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
            return TRUE;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            if (!locked)
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
            return PostMessage(data->hWnd, uMsg, wParam, lParam);
        case WM_ERASEBKGND:
            SetStatus(data, NULL);
            return TRUE;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int LaunchThread(InstanceData *data)
{
    PROCESS_INFORMATION pInfo;
    STARTUPINFOA sInfo;

    memset(&pInfo, 0, sizeof(PROCESS_INFORMATION));
    memset(&sInfo, 0, sizeof(STARTUPINFOA));

    SetStatus(data, "Starting game...");

    char buf[256];
    snprintf(buf, 256, "%d", (unsigned int)data->window.window);
    SetEnvironmentVariable("DDRAW_WINDOW", buf);

    // FIXME: find a way to define the game and it's path
    if (CreateProcessA("redalert\\ra95.exe", NULL, 0, 0, FALSE, CREATE_SUSPENDED, 0, 0, &sInfo, &pInfo))
    {
        HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE, FALSE, pInfo.dwProcessId);

        // don't show the window at all, styles set to zero
        mem_write_dword(hProcess, 0x005B398F, 0x00000000);
        mem_write_byte(hProcess, 0x005B399E, 0x00);
        mem_write_nop(hProcess, 0x005B39A6, 2);
        mem_write_nop(hProcess, 0x005B39BF, 8);

        // forces sound even when the actual game window is inactive
        mem_write_dword(hProcess, 0x005BEC82, 0x00008080);

        ResumeThread(pInfo.hThread);

        SetStatus(data, "Waiting for image...");

        data->hProcess = pInfo.hProcess;
    } else {
        SetStatus(data, "Failed to start the game.");
        is_running = FALSE;
    }

    return TRUE;
}

NPError NPP_SetWindow(NPP instance, NPWindow* window)
{
    InstanceData* data = (InstanceData*)(instance->pdata);

    printf("NPP_SetWindow(instance=%p, window=%p)\n", instance, window);

    data->window = *window;

    SetWindowLong(window->window, GWL_WNDPROC, (LONG)WndProc);
    SetWindowLong(window->window, GWL_USERDATA, (LONG)data);

    if (is_running)
    {
        SetStatus(data, "A game is already running.");
        return NPERR_NO_ERROR;
    }

    is_running = TRUE;

#ifndef NOGAME
    if (data->hThread == NULL && data->window.width && data->window.height)
    {
        data->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)LaunchThread, (void *)data, 0, NULL);
    }
    else
    {
        /* hope that the game is running */
        PostMessage(data->hWnd, WM_USER, 0, (LPARAM)data->window.window);
    }
#endif

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
