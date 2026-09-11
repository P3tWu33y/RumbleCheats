#include <windows.h>
#include <cstdint>
#include "scanner.h"

uintptr_t scanner::find_pattern(uintptr_t dw_start, uintptr_t dw_end, const char* pattern, int result)
{
    const char* pat = pattern;
    int results = 0;
    uintptr_t match = 0;

    for (uintptr_t dw_cur = dw_start; dw_cur < dw_end; dw_cur++)
    {
        __try
        {
            if (!*pat)
                return match;

            if (*(PBYTE)pat == '\?' || *(BYTE*)dw_cur == get_byte(pat))
            {
                if (!match)
                    match = dw_cur;

                if (!pat[2])
                {
                    if (results + 1 != result)
                        results++;
                    else
                        return match;
                }

                if (*(PWORD)pat == '\?\?' || *(PBYTE)pat != '\?')
                    pat += 3;
                else
                    pat += 2;
            }
            else
            {
                pat = pattern;
                match = 0;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    return dw_start;
}
