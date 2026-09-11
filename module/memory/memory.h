#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>

#include "utils.h"  // assuming this defines utils::replace_string and utils::hex_to_bytes

#define ROUND_DOWN(x, align) ((uintptr_t)(x) & ~((uintptr_t)(align) - 1))
#define ROUND_UP(x, align) ((((uintptr_t)(x) + ((uintptr_t)(align) - 1)) & ~((uintptr_t)(align) - 1)))

namespace memapi {

    int __stdcall write(uintptr_t address, const std::string& write_buffer);

    namespace pointer {

        bool valid(uintptr_t base, uintptr_t offset);
        int read_int(uintptr_t base, uintptr_t offset);
        void write_int(uintptr_t base, uintptr_t offset, int value);

        std::string read_str(uintptr_t base, uintptr_t offset);
        void write_str(uintptr_t base, uintptr_t offset, const std::string& value);

        float read_float(uintptr_t base, uintptr_t offset);
        void write_float(uintptr_t base, uintptr_t offset, float value);
    }

    int read_direct(uintptr_t addr);
}
