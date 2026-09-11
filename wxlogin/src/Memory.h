#pragma once

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace ExternalMemory
{
    // -----------------------------------------------------------------
    // Core memory operations (require an open process handle)
    // -----------------------------------------------------------------

    // Read raw bytes from the target process.
    // Returns true on success, false on failure.
    bool ReadBytes(HANDLE hProcess, uintptr_t address, void* buffer, size_t size) noexcept;

    // Write raw bytes to the target process.
    // Returns true on success, false on failure.
    bool WriteBytes(HANDLE hProcess, uintptr_t address, const void* buffer, size_t size) noexcept;

    // Convenience read/write for common types.
    template<typename T>
    bool Read(HANDLE hProcess, uintptr_t address, T& out) noexcept
    {
        return ReadBytes(hProcess, address, &out, sizeof(T));
    }

    template<typename T>
    bool Write(HANDLE hProcess, uintptr_t address, const T& value) noexcept
    {
        return WriteBytes(hProcess, address, &value, sizeof(T));
    }

    // Specialised helpers for clarity.
    inline bool ReadInt32(HANDLE hProcess, uintptr_t address, int32_t& out) noexcept
    {
        return Read(hProcess, address, out);
    }
    inline bool WriteInt32(HANDLE hProcess, uintptr_t address, int32_t value) noexcept
    {
        return Write(hProcess, address, value);
    }

    inline bool ReadUInt32(HANDLE hProcess, uintptr_t address, uint32_t& out) noexcept
    {
        return Read(hProcess, address, out);
    }

    inline bool ReadInt64(HANDLE hProcess, uintptr_t address, int64_t& out) noexcept
    {
        return Read(hProcess, address, out);
    }

    inline bool ReadFloat(HANDLE hProcess, uintptr_t address, float& out) noexcept
    {
        return Read(hProcess, address, out);
    }
    inline bool WriteFloat(HANDLE hProcess, uintptr_t address, float value) noexcept
    {
        return Write(hProcess, address, value);
    }

    inline bool ReadPtr(HANDLE hProcess, uintptr_t address, uintptr_t& out) noexcept
    {
        return Read(hProcess, address, out);
    }

    // Read a null‑terminated ASCII string (max length = maxLen, including null).
    // Returns std::nullopt on failure.
    std::optional<std::string> ReadStringA(HANDLE hProcess, uintptr_t address,
        size_t maxLen = 256) noexcept;

    // Write a null‑terminated ASCII string (including the null terminator).
    // Returns true on success.
    bool WriteStringA(HANDLE hProcess, uintptr_t address, const std::string& str) noexcept;

    // -----------------------------------------------------------------
    // Pointer chain resolution
    // -----------------------------------------------------------------

    // Follow a chain of pointers with offsets.
    // Returns the final address on success, std::nullopt otherwise.
    std::optional<uintptr_t> FollowPointerChain(
        HANDLE hProcess,
        uintptr_t baseAddress,
        const std::vector<ptrdiff_t>& offsets) noexcept;

    // -----------------------------------------------------------------
    // Module helper (get base address of a module)
    // -----------------------------------------------------------------

    // Returns the base address of the specified module in the target process.
    // If moduleName is nullptr, returns the base of the main executable.
    uintptr_t GetModuleBase(HANDLE hProcess, const wchar_t* moduleName = nullptr) noexcept;

    // Returns the base address and size of a module.
    bool GetModuleRange(HANDLE hProcess, const wchar_t* moduleName,
        uintptr_t& outBase, size_t& outSize) noexcept;
}