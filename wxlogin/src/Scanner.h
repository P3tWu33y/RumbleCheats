#pragma once

#include <windows.h>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>
#include <cstddef>

namespace scanner
{
    class Pattern
    {
    public:
        Pattern(std::string_view ida_pattern);

        bool empty() const noexcept { return bytes.empty(); }

        std::vector<int16_t> bytes;
        size_t size{};

        uint8_t skip[256]{};
    };

    uintptr_t find(
        HANDLE hProcess,
        uintptr_t start,
        uintptr_t end,
        const Pattern& pat,
        int nth = 1) noexcept;

    uintptr_t find(
        HANDLE hProcess,
        uintptr_t start,
        uintptr_t end,
        std::string_view ida_pattern,
        int nth = 1);

    std::vector<uintptr_t> find_all(
        HANDLE hProcess,
        uintptr_t start,
        uintptr_t end,
        const Pattern& pat) noexcept;

    uintptr_t find_in_module(
        HANDLE hProcess,
        const wchar_t* module_name,
        std::string_view ida_pattern,
        int nth = 1);

    std::vector<uintptr_t> find_all_in_module(
        HANDLE hProcess,
        const wchar_t* module_name,
        std::string_view ida_pattern);

    uintptr_t resolve_rel32(
        HANDLE hProcess,
        uintptr_t address,
        size_t instruction_size) noexcept;

    std::optional<uintptr_t> follow_ptr_chain(
        HANDLE hProcess,
        uintptr_t base,
        const std::vector<std::ptrdiff_t>& offsets) noexcept;
}