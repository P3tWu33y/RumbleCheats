#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "memory.h"
#include "scanner.h"
#include "bypass.h"
#include "resolver.h"
#include "IPCServer.h"



#define dw_start  0x00400000
#define dw_end    0x00FFFFFF



// ---------------------------------------------------------------------------
// Main thread — runs outside DllMain to avoid loader-lock issues.
// ---------------------------------------------------------------------------

DWORD WINAPI MainThread(LPVOID lpParam)
{
    GameGuard();

    // Used for debugging purposes, otherwise comment this.
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    std::cout << "HelloWorld!" << std::endl;


	//We resolve all the addresses using pattern scanning, this is done to avoid hardcoding addresses that may change with updates.
    resolveAddresses();

    StartIPCServer();

    return 0;
}

// ---------------------------------------------------------------------------
// DllMain
// ---------------------------------------------------------------------------
extern HINSTANCE hAppInstance;
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CloseHandle(CreateThread(nullptr, 0, MainThread, lpReserved, 0, nullptr));
    }
    return TRUE;
}