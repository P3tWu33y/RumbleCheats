#pragma once

#include <windows.h>

#include <string>
#include <vector>

struct ProcessEntry
{
    std::wstring name;
    DWORD pid = 0;
};

namespace ProcessUtils
{
    // Enumerates all currently running processes and returns every one
    // whose executable name matches processName (case-insensitive,
    // e.g. L"MU.exe").
    std::vector<ProcessEntry> FindProcessesByName(const std::wstring& processName);

    // Convenience formatter used by the dropdown: "MU.exe (PID: 1234)"
    std::wstring FormatEntry(const ProcessEntry& entry);
}
