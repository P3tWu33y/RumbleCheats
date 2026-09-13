#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wx/wx.h>
#include <wx/sysopt.h>
#include "App.h"
#include "memory.h"
#include "scanner.h"
#include "bypass.h"
#include "resolver.h"


#define dw_start  0x00400000
#define dw_end    0x00FFFFFF

// ---------------------------------------------------------------------------
// Writes a temporary manifest and activates a Common Controls v6 context.
// Returns the context handle — caller must ReleaseActCtx() when done.
// ---------------------------------------------------------------------------
static HANDLE ActivateCommCtrl6()
{
    static const char kManifest[] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
        "<dependency><dependentAssembly>"
        "<assemblyIdentity type=\"win32\""
        " name=\"Microsoft.Windows.Common-Controls\""
        " version=\"6.0.0.0\""
        " processorArchitecture=\"*\""
        " publicKeyToken=\"6595b64144ccf1df\""
        " language=\"*\"/>"
        "</dependentAssembly></dependency>"
        "</assembly>";

    wchar_t tmpDir[MAX_PATH];
    wchar_t tmpFile[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpDir);
    GetTempFileNameW(tmpDir, L"mfst", 0, tmpFile);
    wcscat_s(tmpFile, L".manifest");

    HANDLE hFile = CreateFileW(tmpFile, GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return nullptr;

    DWORD written;
    WriteFile(hFile, kManifest, sizeof(kManifest) - 1, &written, nullptr);
    CloseHandle(hFile);

    ACTCTXW ctx = { sizeof(ctx) };
    ctx.lpSource = tmpFile;

    HANDLE hActCtx = CreateActCtxW(&ctx);
    DeleteFileW(tmpFile);

    if (hActCtx == INVALID_HANDLE_VALUE)
        return nullptr;

    ULONG_PTR cookie;
    if (!ActivateActCtx(hActCtx, &cookie))
    {
        ReleaseActCtx(hActCtx);
        return nullptr;
    }

    return hActCtx;
}

// ---------------------------------------------------------------------------
// Main thread — runs outside DllMain to avoid loader-lock issues.
// ---------------------------------------------------------------------------
static DWORD WINAPI MainThread(LPVOID lpParam)
{
    Sleep(2500);

    GameGuard();

    //AllocConsole();
    //freopen("CONOUT$", "w", stdout);
    //std::cout << "HelloWorld!" << std::endl;

   
 //   const char* LobbyIndex_Pattern = "8B 0D ?? ?? ?? 00 8B 15 ?? ?? ?? 00 8B 0C 8A E8 ?? ?? ?? FF 68 ?? ?? ?? 00 8D 8D 44 FC FF FF E8 ?? ?? ?? 00";
	//uintptr_t lobby_Address = scanner::find_pattern(dw_start, dw_end, LobbyIndex_Pattern, 1);

 //   if (lobby_Address != 0 || lobby_Address != dw_start) {
 //       std::cout << "Pattern found at address: 0x" << std::hex << lobby_Address << std::endl;
 //   }


	//We resolve all the addresses using pattern scanning, this is done to avoid hardcoding addresses that may change with updates.
    resolveAddresses();


    HANDLE hActCtx = ActivateCommCtrl6();

    // The game may have already loaded comctl32 v5 before injection.
    // The activation context above redirects our control creation to v6,
    // but wxWidgets' internal version check will still fail in Debug.
    // This option suppresses that diagnostic — no real init is skipped.
    wxSystemOptions::SetOption(wxT("msw.no-manifest-check"), 1);

    // wxWidgets requires a loader-registered HINSTANCE for window class
    // registration — pass the host EXE, not our manual-map base address.
    HINSTANCE hHost = GetModuleHandleW(nullptr);
    wxEntry(hHost, nullptr, (wxCmdLineArgType)L"", SW_SHOW);

    if (hActCtx)
        ReleaseActCtx(hActCtx);

    return 0;
}

// ---------------------------------------------------------------------------
// DllMain
// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CloseHandle(CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr));
    }

    return TRUE;
}