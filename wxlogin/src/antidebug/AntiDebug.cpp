#include "AntiDebug.h"

#include <windows.h>
#include <winternl.h>
#include <iostream>
#include <iomanip>

#pragma comment(lib, "ntdll.lib")

namespace
{
    // ------------------------------------------------------------------
    // Debugger detection
    // ------------------------------------------------------------------

    typedef NTSTATUS(NTAPI* NtQueryInformationProcess_t)(
        HANDLE ProcessHandle,
        PROCESSINFOCLASS ProcessInformationClass,
        PVOID ProcessInformation,
        ULONG ProcessInformationLength,
        PULONG ReturnLength
        );

    bool CheckNtQueryDebugPort()
    {
        const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");

        if (!ntdll)
            return false;

        const auto NtQueryInformationProcess =
            reinterpret_cast<NtQueryInformationProcess_t>(
                GetProcAddress(ntdll, "NtQueryInformationProcess")
                );

        if (!NtQueryInformationProcess)
            return false;

        DWORD_PTR debugPort = 0;
        ULONG returnLength = 0;

        const NTSTATUS status = NtQueryInformationProcess(
            GetCurrentProcess(),
            ProcessDebugPort,
            &debugPort,
            sizeof(debugPort),
            &returnLength
        );

        // A non-zero debug port means a debugger is attached.
        return status == 0 && debugPort != 0;
    }

    bool CheckRemoteDebugger()
    {
        BOOL present = FALSE;

        if (!CheckRemoteDebuggerPresent(GetCurrentProcess(), &present))
            return false;

        return present != FALSE;
    }

    // ------------------------------------------------------------------
    // CRC32 (standard poly 0xEDB88320)
    // ------------------------------------------------------------------

    uint32_t Crc32(const uint8_t* data, size_t length)
    {
        static uint32_t table[256];
        static bool tableInitialized = false;

        if (!tableInitialized)
        {
            for (uint32_t i = 0; i < 256; ++i)
            {
                uint32_t c = i;

                for (int j = 0; j < 8; ++j)
                {
                    c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
                }

                table[i] = c;
            }

            tableInitialized = true;
        }

        uint32_t crc = 0xFFFFFFFFu;

        for (size_t i = 0; i < length; ++i)
        {
            crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
        }

        return crc ^ 0xFFFFFFFFu;
    }

    // ------------------------------------------------------------------
    // Code-section integrity
    // ------------------------------------------------------------------

    // TODO: Replace with the real CRC32 of your compiled .text section.
    // See the instructions below AntiDebug.cpp for how to compute this.
    constexpr uint32_t kExpectedTextCrc = 0x00000000u;

    bool LocateTextSection(uint8_t*& base, size_t& size)
    {
        const HMODULE moduleHandle = GetModuleHandleW(nullptr);

        if (!moduleHandle)
            return false;

        const auto dosHeader =
            reinterpret_cast<PIMAGE_DOS_HEADER>(moduleHandle);

        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
            return false;

        const auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<uint8_t*>(moduleHandle) + dosHeader->e_lfanew
            );

        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
            return false;

        const PIMAGE_SECTION_HEADER sectionHeader =
            IMAGE_FIRST_SECTION(ntHeaders);

        for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i)
        {
            const auto& section = sectionHeader[i];

            if (memcmp(section.Name, ".text", 5) == 0)
            {
                base = reinterpret_cast<uint8_t*>(moduleHandle) +
                    section.VirtualAddress;

                size = section.Misc.VirtualSize;

                return true;
            }
        }

        return false;
    }
}

namespace AntiDebug
{
    bool IsDebuggerDetected()
    {
        // Any one of these firing is treated as a positive detection.
        // Layering multiple checks means an attacker has to defeat all
        // of them, not just patch out a single IsDebuggerPresent call.
        if (::IsDebuggerPresent())
            return true;

        if (CheckRemoteDebugger())
            return true;

        if (CheckNtQueryDebugPort())
            return true;

        return false;
    }

    bool VerifyCodeIntegrity()
    {
        uint8_t* base = nullptr;
        size_t size = 0;

        if (!LocateTextSection(base, size))
            return false; // Can't verify -- treat as failure, not a pass.

        const uint32_t actualCrc = Crc32(base, size);

        std::cout << "Actual .text CRC: 0x"
            << std::hex << std::uppercase
            << std::setfill('0') << std::setw(8)
            << actualCrc
            << std::dec << std::endl;

        return actualCrc == kExpectedTextCrc;
    }

    bool SecurityCheckPassed()
    {
        if (IsDebuggerDetected())
            return false;

        if (!VerifyCodeIntegrity())
            return false;

        return true;
    }
}



/*

    1. How to fill in kExpectedTextCrc (one-time, and again after every rebuild where .text changes):

    2. Build your release binary once with kExpectedTextCrc = 0x00000000u.
       Add a temporary debug line right after computing actualCrc in VerifyCodeIntegrity():

    3. Run it once, copy the printed value into kExpectedTextCrc, remove the debug line, rebuild.

*/