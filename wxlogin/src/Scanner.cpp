#include "scanner.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>
#include <stdexcept>
#include <vector>
#include <string_view>
#include <optional>
#include <cstddef>      // for ptrdiff_t

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "kernel32.lib")

namespace scanner
{

    // ---------------------------------------------------------------------------
    // Pattern construction (unchanged)
    // ---------------------------------------------------------------------------

    static uint8_t hex_nibble(char c)
    {
        if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
        return 0;
    }

    Pattern::Pattern(std::string_view ida_pattern)
    {
        const char* p = ida_pattern.data();
        const char* end = p + ida_pattern.size();

        while (p < end)
        {
            if (*p == ' ') { ++p; continue; }

            if (*p == '?')
            {
                bytes.push_back(-1);
                ++p;
                if (p < end && *p == '?') ++p;
            }
            else if (p + 1 < end)
            {
                const uint8_t hi = hex_nibble(p[0]);
                const uint8_t lo = hex_nibble(p[1]);
                bytes.push_back(static_cast<int16_t>((hi << 4) | lo));
                p += 2;
            }
            else break;
        }

        size = bytes.size();
        if (size == 0) return;

        std::fill(std::begin(skip), std::end(skip), static_cast<uint8_t>(
            size < 255 ? size : 255));

        for (size_t i = 0; i + 1 < size; ++i)
        {
            if (bytes[i] != -1)
            {
                const size_t dist = size - 1 - i;
                skip[static_cast<uint8_t>(bytes[i])] =
                    static_cast<uint8_t>(dist < 255 ? dist : 255);
            }
        }
    }

    // ---------------------------------------------------------------------------
    // External memory reader
    // ---------------------------------------------------------------------------

    static bool read_process_memory(HANDLE hProcess, uintptr_t address,
        void* buffer, size_t size) noexcept
    {
        SIZE_T bytesRead;
        return ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address),
            buffer, size, &bytesRead) != FALSE
            && bytesRead == size;
    }

    // ---------------------------------------------------------------------------
    // Core scan (external)
    // ---------------------------------------------------------------------------

    uintptr_t find(HANDLE hProcess,
        uintptr_t      start,
        uintptr_t      end,
        const Pattern& pat,
        int            nth) noexcept
    {
        if (pat.empty() || start >= end || nth <= 0)
            return 0;

        const size_t region_size = end - start;
        if (region_size < pat.size)
            return 0;

        std::vector<uint8_t> buffer(region_size);
        if (!read_process_memory(hProcess, start, buffer.data(), region_size))
            return 0;

        const uint8_t* data = buffer.data();
        const size_t scan_end = region_size - pat.size;
        const size_t pat_size = pat.size;
        const int16_t* pat_bytes = pat.bytes.data();

        int hits = 0;
        for (size_t offset = 0; offset <= scan_end; ++offset)
        {
            const uint8_t last_byte = data[offset + pat_size - 1];

            if (pat_bytes[pat_size - 1] != -1 &&
                last_byte != static_cast<uint8_t>(pat_bytes[pat_size - 1]))
            {
                offset += pat.skip[last_byte] - 1;  // -1 because loop will increment
                continue;
            }

            bool found = true;
            for (size_t j = 0; j < pat_size - 1; ++j)
            {
                if (pat_bytes[j] == -1) continue;
                if (data[offset + j] != static_cast<uint8_t>(pat_bytes[j]))
                {
                    found = false;
                    break;
                }
            }

            if (found)
            {
                ++hits;
                if (hits == nth)
                    return start + offset;
            }
        }

        return 0;
    }

    uintptr_t find(HANDLE hProcess,
        uintptr_t        start,
        uintptr_t        end,
        std::string_view ida_pattern,
        int              nth)
    {
        return find(hProcess, start, end, Pattern{ ida_pattern }, nth);
    }

    std::vector<uintptr_t> find_all(HANDLE hProcess,
        uintptr_t      start,
        uintptr_t      end,
        const Pattern& pat) noexcept
    {
        std::vector<uintptr_t> results;
        if (pat.empty() || start >= end) return results;

        const size_t region_size = end - start;
        if (region_size < pat.size) return results;

        std::vector<uint8_t> buffer(region_size);
        if (!read_process_memory(hProcess, start, buffer.data(), region_size))
            return results;

        const uint8_t* data = buffer.data();
        const size_t scan_end = region_size - pat.size;
        const size_t pat_size = pat.size;
        const int16_t* pat_bytes = pat.bytes.data();

        for (size_t offset = 0; offset <= scan_end; ++offset)
        {
            const uint8_t last_byte = data[offset + pat_size - 1];

            if (pat_bytes[pat_size - 1] != -1 &&
                last_byte != static_cast<uint8_t>(pat_bytes[pat_size - 1]))
            {
                offset += pat.skip[last_byte] - 1;
                continue;
            }

            bool found = true;
            for (size_t j = 0; j < pat_size - 1; ++j)
            {
                if (pat_bytes[j] == -1) continue;
                if (data[offset + j] != static_cast<uint8_t>(pat_bytes[j]))
                {
                    found = false;
                    break;
                }
            }

            if (found)
                results.push_back(start + offset);
        }

        return results;
    }

    // ---------------------------------------------------------------------------
    // Module helpers (external)
    // ---------------------------------------------------------------------------

    static std::pair<uintptr_t, uintptr_t> module_range(HANDLE hProcess,
        const wchar_t* name)
    {
        const DWORD pid = GetProcessId(hProcess);
        if (pid == 0) return { 0, 0 };

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snapshot == INVALID_HANDLE_VALUE) return { 0, 0 };

        MODULEENTRY32W me32;
        me32.dwSize = sizeof(MODULEENTRY32W);

        bool found = false;
        uintptr_t base = 0, size = 0;

        if (Module32FirstW(snapshot, &me32))
        {
            do
            {
                if (name == nullptr)
                {
                    // First module is typically the main executable
                    found = true;
                    base = reinterpret_cast<uintptr_t>(me32.modBaseAddr);
                    size = me32.modBaseSize;
                    break;
                }
                else if (_wcsicmp(me32.szModule, name) == 0)
                {
                    found = true;
                    base = reinterpret_cast<uintptr_t>(me32.modBaseAddr);
                    size = me32.modBaseSize;
                    break;
                }
            } while (Module32NextW(snapshot, &me32));
        }

        CloseHandle(snapshot);
        if (!found) return { 0, 0 };

        return { base, base + size };
    }

    uintptr_t find_in_module(HANDLE hProcess,
        const wchar_t* module_name,
        std::string_view ida_pattern,
        int              nth)
    {
        auto [start, end] = module_range(hProcess, module_name);
        if (!start) return 0;
        return find(hProcess, start, end, Pattern{ ida_pattern }, nth);
    }

    std::vector<uintptr_t> find_all_in_module(HANDLE hProcess,
        const wchar_t* module_name,
        std::string_view ida_pattern)
    {
        auto [start, end] = module_range(hProcess, module_name);
        if (!start) return {};
        return find_all(hProcess, start, end, Pattern{ ida_pattern });
    }

    // ---------------------------------------------------------------------------
    // Typed helpers (external)
    // ---------------------------------------------------------------------------

    uintptr_t resolve_rel32(
        HANDLE hProcess,
        uintptr_t addr,
        size_t instruction_size) noexcept
    {
        int32_t displacement{};

        SIZE_T bytesRead = 0;

        if (!ReadProcessMemory(
            hProcess,
            reinterpret_cast<LPCVOID>(addr + instruction_size - 4),
            &displacement,
            sizeof(displacement),
            &bytesRead) ||
            bytesRead != sizeof(displacement))
        {
            return 0;
        }

        return static_cast<uintptr_t>(
            static_cast<int64_t>(addr) +
            static_cast<int64_t>(instruction_size) +
            static_cast<int64_t>(displacement)
            );
    }

    // Changed signature: std::span → const std::vector<ptrdiff_t>&
    std::optional<uintptr_t> follow_ptr_chain(
        HANDLE                       hProcess,
        uintptr_t                    base,
        const std::vector<ptrdiff_t>& offsets) noexcept
    {
        uintptr_t ptr = base;
        for (ptrdiff_t offset : offsets)
        {
            if (!ptr) return std::nullopt;
            uintptr_t next;
            if (!read_process_memory(hProcess, ptr, &next, sizeof(next)))
                return std::nullopt;
            ptr = next + offset;
        }
        return ptr;
    }

} // namespace scanner