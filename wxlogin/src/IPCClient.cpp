#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <atomic>

#include <wx/wx.h>

#include "IPCClient.h"

namespace
{
    constexpr const wchar_t* PIPE_NAME =
        LR"(\\.\pipe\WussysToolIPC)";

    HANDLE g_pipe = INVALID_HANDLE_VALUE;

    std::atomic<bool> g_ipcRunning{ false };
    HANDLE g_monitorThread = nullptr;
}

DWORD WINAPI IPCMonitorThread(LPVOID)
{
    while (g_ipcRunning)
    {
        IPCMessage message{};
        DWORD bytesRead = 0;

        HANDLE pipe = g_pipe;

        if (pipe == INVALID_HANDLE_VALUE)
            break;

        BOOL result = ReadFile(
            pipe,
            &message,
            sizeof(message),
            &bytesRead,
            nullptr
        );

        if (!result || bytesRead != sizeof(message))
        {
            if (g_ipcRunning.exchange(false))
            {
                wxTheApp->CallAfter([]()
                    {
                        wxTheApp->ExitMainLoop();
                    });
            }

            break;
        }

        if (message.type == IPCMessageType::Heartbeat)
            continue;
    }

    return 0;
}

bool ConnectIPC()
{
    if (g_pipe != INVALID_HANDLE_VALUE)
        return true;

    if (!WaitNamedPipeW(PIPE_NAME, 100))
        return false;

    HANDLE pipe = CreateFileW(
        PIPE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (pipe == INVALID_HANDLE_VALUE)
        return false;

    g_pipe = pipe;
    g_ipcRunning = true;

    g_monitorThread = CreateThread(
        nullptr,
        0,
        IPCMonitorThread,
        nullptr,
        0,
        nullptr
    );

    if (!g_monitorThread)
    {
        g_ipcRunning = false;

        CloseHandle(g_pipe);
        g_pipe = INVALID_HANDLE_VALUE;

        return false;
    }

    return true;
}

void DisconnectIPC()
{
    g_ipcRunning = false;

    if (g_pipe != INVALID_HANDLE_VALUE)
    {
        CancelIoEx(g_pipe, nullptr);
    }

    if (g_monitorThread)
    {
        WaitForSingleObject(g_monitorThread, 1000);

        CloseHandle(g_monitorThread);
        g_monitorThread = nullptr;
    }

    if (g_pipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_pipe);
        g_pipe = INVALID_HANDLE_VALUE;
    }
}

bool SetFeature(IPCCommand command, bool enabled)
{
    if (g_pipe == INVALID_HANDLE_VALUE)
    {
        if (!ConnectIPC())
            return false;
    }

    IPCMessage message{};

    message.type = IPCMessageType::Feature;
    message.command = command;
    message.enabled = enabled;

    DWORD bytesWritten = 0;

    BOOL result = WriteFile(
        g_pipe,
        &message,
        sizeof(message),
        &bytesWritten,
        nullptr
    );

    if (!result || bytesWritten != sizeof(message))
    {
        DisconnectIPC();
        return false;
    }

    return true;
}