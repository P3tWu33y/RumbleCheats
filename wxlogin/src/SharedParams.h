#pragma once
#include <Windows.h>

// Shared between injector and DLL.
// Must be identical on both sides — do not add pointers or STL types.
// All strings are fixed-size arrays to keep layout flat and copyable
// across process boundaries via WriteProcessMemory.

struct SharedParams
{
    char  username[64];   // logged-in user from Firebase auth
    DWORD reserved;       // pad to 8-byte boundary, use later if needed
};