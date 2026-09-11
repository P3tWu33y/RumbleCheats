#include "ProcessUtils.h"

#include <tlhelp32.h>

#include <algorithm>
#include <cwctype>

namespace
{
    bool IEquals(const std::wstring& a, const std::wstring& b)
    {
        if (a.size() != b.size())
            return false;

        return std::equal(a.begin(), a.end(), b.begin(), [](wchar_t l, wchar_t r)
        {
            return std::towlower(l) == std::towlower(r);
        });
    }
}

namespace ProcessUtils
{
    std::vector<ProcessEntry> FindProcessesByName(const std::wstring& processName)
    {
        std::vector<ProcessEntry> results;

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (snapshot == INVALID_HANDLE_VALUE)
            return results;

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        if (Process32FirstW(snapshot, &entry))
        {
            do
            {
                if (IEquals(entry.szExeFile, processName))
                {
                    results.push_back({ entry.szExeFile, entry.th32ProcessID });
                }
            } while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);

        return results;
    }

    std::wstring FormatEntry(const ProcessEntry& entry)
    {
        return entry.name + L" (PID: " + std::to_wstring(entry.pid) + L")";
    }
}
