#include "memory.h"
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <cstring>

namespace ExternalMemory
{
    // -----------------------------------------------------------------
    // Core read/write
    // -----------------------------------------------------------------

    bool ReadBytes(HANDLE hProcess, uintptr_t address, void* buffer, size_t size) noexcept
    {
        if (!hProcess || !buffer || !size)
            return false;

        SIZE_T bytesRead;
        return ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address),
            buffer, size, &bytesRead) != FALSE
            && bytesRead == size;
    }

    bool WriteBytes(HANDLE hProcess, uintptr_t address, const void* buffer, size_t size) noexcept
    {
        if (!hProcess || !buffer || !size)
            return false;

        SIZE_T bytesWritten;
        return WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(address),
            buffer, size, &bytesWritten) != FALSE
            && bytesWritten == size;
    }

    // -----------------------------------------------------------------
    // String read/write
    // -----------------------------------------------------------------

    std::optional<std::string> ReadStringA(HANDLE hProcess, uintptr_t address,
        size_t maxLen) noexcept
    {
        if (!hProcess || !address || maxLen == 0)
            return std::nullopt;

        std::vector<char> buffer(maxLen);
        if (!ReadBytes(hProcess, address, buffer.data(), maxLen))
            return std::nullopt;

        // Ensure null-termination
        buffer[maxLen - 1] = '\0';
        return std::string(buffer.data());
    }

    bool WriteStringA(HANDLE hProcess, uintptr_t address, const std::string& str) noexcept
    {
        // Write the whole string including the null terminator
        return WriteBytes(hProcess, address, str.c_str(), str.size() + 1);
    }

    // -----------------------------------------------------------------
    // Pointer chain
    // -----------------------------------------------------------------

    std::optional<uintptr_t> FollowPointerChain(
        HANDLE hProcess,
        uintptr_t baseAddress,
        const std::vector<ptrdiff_t>& offsets) noexcept
    {
        uintptr_t current = baseAddress;
        for (ptrdiff_t offset : offsets)
        {
            if (!current)
                return std::nullopt;

            uintptr_t nextPtr;
            if (!ReadBytes(hProcess, current, &nextPtr, sizeof(nextPtr)))
                return std::nullopt;

            current = nextPtr + offset;
        }
        return current;
    }

    // -----------------------------------------------------------------
    // Module helpers
    // -----------------------------------------------------------------

    bool GetModuleRange(HANDLE hProcess, const wchar_t* moduleName,
        uintptr_t& outBase, size_t& outSize) noexcept
    {
        DWORD pid = GetProcessId(hProcess);
        if (pid == 0)
            return false;

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snapshot == INVALID_HANDLE_VALUE)
            return false;

        MODULEENTRY32W me32;
        me32.dwSize = sizeof(MODULEENTRY32W);

        bool found = false;
        if (Module32FirstW(snapshot, &me32))
        {
            do
            {
                if (moduleName == nullptr || _wcsicmp(me32.szModule, moduleName) == 0)
                {
                    outBase = reinterpret_cast<uintptr_t>(me32.modBaseAddr);
                    outSize = me32.modBaseSize;
                    found = true;
                    break;
                }
            } while (Module32NextW(snapshot, &me32));
        }

        CloseHandle(snapshot);
        return found;
    }

    uintptr_t GetModuleBase(HANDLE hProcess, const wchar_t* moduleName) noexcept
    {
        uintptr_t base;
        size_t size;
        if (GetModuleRange(hProcess, moduleName, base, size))
            return base;
        return 0;
    }
}