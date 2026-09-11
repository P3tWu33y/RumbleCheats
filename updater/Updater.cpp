#include <windows.h>

#include <string>

int wmain(int argc, wchar_t* argv[])
{

    if (argc < 4)
        return 1;

    const std::wstring downloadedExePath = argv[1];
    const std::wstring targetExePath = argv[2];
    const DWORD parentPid =
        static_cast<DWORD>(_wtoi(argv[3]));

    // Wait for the main app to fully exit before touching its exe file.
    HANDLE parentProcess =
        OpenProcess(SYNCHRONIZE, FALSE, parentPid);

    if (parentProcess)
    {
        WaitForSingleObject(parentProcess, 10000); // up to 10s
        CloseHandle(parentProcess);
    }

    // Give the OS a moment to release the file handle fully.
    Sleep(500);

    // Replace the old exe with the newly downloaded one.
    if (!MoveFileExW(
        downloadedExePath.c_str(),
        targetExePath.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
    {
        const DWORD error = GetLastError();

        wchar_t message[256]{};
        swprintf_s(message, L"Failed to install update.\nError code: %lu", error);

        MessageBoxW(nullptr, message, L"Update Error", MB_ICONERROR);

        return 1;
    }

    // Relaunch the updated app.
    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo{};

    CreateProcessW(
        targetExePath.c_str(),
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo
    );

    if (processInfo.hProcess)
        CloseHandle(processInfo.hProcess);

    if (processInfo.hThread)
        CloseHandle(processInfo.hThread);

    return 0;
}