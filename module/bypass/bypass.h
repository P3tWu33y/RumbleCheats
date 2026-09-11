#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <psapi.h>
#include "scanner.h"
#include "memory.h"
#include <winternl.h>
#include "suspender.h"
#include "utils.h"

#pragma comment(lib, "ntdll.lib") // Or load dynamically with GetProcAddress
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "advapi32.lib")

void BypassExample();
void GameGuard();

