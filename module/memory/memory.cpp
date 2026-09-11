#include "memory.h"

int __stdcall memapi::write(uintptr_t address, const std::string& write_buffer_input)
{
    DWORD old_protection_flags;
    SIZE_T size;
    void* target_address = reinterpret_cast<void*>(address);

    // Copy buffer to modify it safely
    std::string write_buffer = write_buffer_input;

    // Calculate size: number of bytes (count hex pairs)
    size = std::count(write_buffer.begin(), write_buffer.end(), ' ') + 1;

    utils::replace_string(write_buffer, " ", "");
    std::vector<unsigned char> bytes = utils::hex_to_bytes(write_buffer);
    PBYTE write_data = bytes.data();

    uintptr_t page_start = ROUND_DOWN(target_address, 0x1000);
    SIZE_T page_size = ROUND_UP(size, 0x1000);

    if (VirtualProtect(reinterpret_cast<LPVOID>(page_start), page_size, PAGE_EXECUTE_READWRITE, &old_protection_flags))
    {
        memcpy(target_address, write_data, size);

        DWORD temp_protect;
        VirtualProtect(reinterpret_cast<LPVOID>(page_start), page_size, old_protection_flags, &temp_protect);

        if (FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(page_start), page_size))
            return 0;

        return GetLastError();
    }

    return GetLastError();
}

bool memapi::pointer::valid(uintptr_t base, uintptr_t offset)
{
    uintptr_t* pBase = reinterpret_cast<uintptr_t*>(base);

    __try {
        if (!IsBadReadPtr(pBase, sizeof(uintptr_t)))
        {
            uintptr_t ptr = *pBase + offset;
            if (!IsBadReadPtr(reinterpret_cast<void*>(ptr), sizeof(uintptr_t)))
                return true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    return false;
}

int memapi::pointer::read_int(uintptr_t base, uintptr_t offset)
{
    if (valid(base, offset))
        return *reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(base) + offset);
    return 0;
}

void memapi::pointer::write_int(uintptr_t base, uintptr_t offset, int value)
{
    if (valid(base, offset))
        *reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(base) + offset) = value;
}

std::string memapi::pointer::read_str(uintptr_t base, uintptr_t offset)
{
    if (valid(base, offset))
        return *reinterpret_cast<std::string*>(*reinterpret_cast<uintptr_t*>(base) + offset);
    return "";
}

void memapi::pointer::write_str(uintptr_t base, uintptr_t offset, const std::string& value)
{
    if (valid(base, offset))
        *reinterpret_cast<std::string*>(*reinterpret_cast<uintptr_t*>(base) + offset) = value;
}

float memapi::pointer::read_float(uintptr_t base, uintptr_t offset)
{
    if (valid(base, offset))
        return *reinterpret_cast<float*>(*reinterpret_cast<uintptr_t*>(base) + offset);
    return 0.0f;
}

void memapi::pointer::write_float(uintptr_t base, uintptr_t offset, float value)
{
    if (valid(base, offset))
        *reinterpret_cast<float*>(*reinterpret_cast<uintptr_t*>(base) + offset) = value;
}

int memapi::read_direct(uintptr_t addr)
{
    return *reinterpret_cast<int*>(addr);
}
