#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>

#include "IPCServer.h"
#include "IPC.h"
#include "memory.h"
#include "resolver.h"



uintptr_t returnAddress = 0;

__declspec(naked) void SetESI5()
{
    __asm
    {
        mov esi, 0x4
        test esi, esi
        jmp returnAddress
    }
}


namespace
{
    constexpr const wchar_t* PIPE_NAME =
        LR"(\\.\pipe\WussysToolIPC)";


    // =========================================================
	// Handle heartbeat messages to keep the connection alive
    // =========================================================


    DWORD WINAPI HeartbeatThread(LPVOID param)
    {
        HANDLE pipe = static_cast<HANDLE>(param);

        while (true)
        {
            Sleep(1000);

            IPCMessage heartbeat{};

            heartbeat.type = IPCMessageType::Heartbeat;
            heartbeat.command = IPCCommand::Feature1;
            heartbeat.enabled = false;

            DWORD bytesWritten = 0;

            BOOL result = WriteFile(
                pipe,
                &heartbeat,
                sizeof(heartbeat),
                &bytesWritten,
                nullptr
            );

            if (!result || bytesWritten != sizeof(heartbeat))
            {
                break;
            }
        }

        return 0;
    }


    // =========================================================
    // Handle messages received from the client
    // =========================================================

    void HandleMessage(const IPCMessage& message)
    {
        switch (message.command)
        {
        case IPCCommand::Feature1:
        {
            if (message.enabled)
            {
                std::cout << "[IPC] Feature1 ENABLED"
                    << std::endl;

                // =============================================
                // FEATURE 1 ENABLE LOGIC
                // =============================================

                // Your logic here.

                memapi::write(KillAll, "EB"); // This will make massive amount of hits. -- Disabled for public release.


                WriteJmp(KillAll + 0x61, Hit);
                WriteJmp(KillAll + 0x73, Hit);

                WriteJmp(BossKO, KillBossFunc);
                WriteJmp(MonstersKO, KillMonsterFunc);

            }
            else
            {
                std::cout << "[IPC] Feature1 DISABLED"
                    << std::endl;

                // =============================================
                // FEATURE 1 DISABLE LOGIC
                // =============================================

                // Your cleanup logic here.

                memapi::write(KillAll, "75"); // This will make massive amount of hits. -- Disabled for public release.

                WriteJe(KillAll + 0x61, Hit + 0x1C);
                WriteJe(KillAll + 0x73, Hit + 0x2E);

                WriteJng(BossKO, KillBossFunc);
                WriteJng(MonstersKO, KillMonsterFunc);



            }

            break;
        }


        case IPCCommand::Feature2:
        {
            if (message.enabled)
            {
                std::cout << "[IPC] Feature2 ENABLED"
                    << std::endl;

                // =============================================
                // FEATURE 2 ENABLE LOGIC
                // =============================================

                // Your logic here.

                uintptr_t OriginalChestHack = ChestHack;
                returnAddress = ChestHack += 0x5;

                WriteJmp(OriginalChestHack, (uintptr_t)&SetESI5);


            }
            else
            {
                std::cout << "[IPC] Feature2 DISABLED"
                    << std::endl;

                // =============================================
                // FEATURE 2 DISABLE LOGIC
                // =============================================

                // Your cleanup logic here.

                memapi::write(ChestHack, "8B 75 A4 85 F6");
            }

            break;
        }


        default:
        {
            std::cout << "[IPC] Unknown command"
                << std::endl;

            break;
        }
        }
    }


    // =========================================================
    // IPC Server Thread
    // =========================================================

    DWORD WINAPI IPCThread(LPVOID)
    {
        while (true)
        {
            HANDLE pipe = CreateNamedPipeW(
                PIPE_NAME,

                PIPE_ACCESS_DUPLEX,

                PIPE_TYPE_MESSAGE |
                PIPE_READMODE_MESSAGE |
                PIPE_WAIT,

                1,

                sizeof(IPCMessage),
                sizeof(IPCMessage),

                0,

                nullptr
            );


            if (pipe == INVALID_HANDLE_VALUE)
            {
                std::cout
                    << "[IPC] CreateNamedPipe failed. Error: "
                    << GetLastError()
                    << std::endl;

                return 1;
            }


            std::cout
                << "[IPC] Waiting for client..."
                << std::endl;


            BOOL connected = ConnectNamedPipe(
                pipe,
                nullptr
            );


            if (!connected)
            {
                DWORD error = GetLastError();

                if (error != ERROR_PIPE_CONNECTED)
                {
                    CloseHandle(pipe);
                    continue;
                }
            }


            std::cout
                << "[IPC] Client connected."
                << std::endl;


            HANDLE heartbeatThread = CreateThread(
                nullptr,
                0,
                HeartbeatThread,
                pipe,
                0,
                nullptr
            );

            if (heartbeatThread)
            {
                CloseHandle(heartbeatThread);
            }


            // =============================================
            // Keep receiving messages while client is connected
            // =============================================

            while (true)
            {
                IPCMessage message{};

                DWORD bytesRead = 0;


                BOOL result = ReadFile(
                    pipe,
                    &message,
                    sizeof(message),
                    &bytesRead,
                    nullptr
                );


                if (!result ||
                    bytesRead != sizeof(message))
                {
                    break;
                }


                HandleMessage(message);
            }


            std::cout
                << "[IPC] Client disconnected."
                << std::endl;


            DisconnectNamedPipe(pipe);
            CloseHandle(pipe);
        }

        return 0;
    }
}


// =========================================================
// Start the IPC server
// =========================================================

void StartIPCServer()
{
    HANDLE thread = CreateThread(
        nullptr,
        0,
        IPCThread,
        nullptr,
        0,
        nullptr
    );

    if (thread)
    {
        CloseHandle(thread);
    }
}