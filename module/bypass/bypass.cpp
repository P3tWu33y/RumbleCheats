#include "bypass.h"

typedef LONG NTSTATUS_S;
#define THREAD_QUERY_WIN32_ADDR 9
typedef NTSTATUS_S(WINAPI* pNtQueryInfoThread)(HANDLE, int, PVOID, ULONG, PULONG);




void BypassExample() {

	HMODULE hLG = NULL;
	while (!hLG) {
		hLG = GetModuleHandleA("protect.dll");
		Sleep(200);
	}

	std::cout << "[+] protect.dll detected, Waiting for stability..." << std::endl;
	//Sleep(5000);

	MODULEINFO mi;
	if (GetModuleInformation(GetCurrentProcess(), hLG, &mi, sizeof(mi))) {
		DWORD_PTR dwBase = (DWORD_PTR)mi.lpBaseOfDll;
		DWORD_PTR dwEnd = dwBase + mi.SizeOfImage;

		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hSnap != INVALID_HANDLE_VALUE) {
			THREADENTRY32 te = { sizeof(te) };
			te.dwSize = sizeof(te);
			if (Thread32First(hSnap, &te)) {
				do {
					if (te.th32OwnerProcessID == GetCurrentProcessId() && te.th32ThreadID != GetCurrentThreadId()) {
						HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
						if (hThread) {
							DWORD_PTR startAddr = 0;
							auto NtQIT = (pNtQueryInfoThread)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationThread");
							if (NtQIT) {
								NtQIT(hThread, THREAD_QUERY_WIN32_ADDR, &startAddr, sizeof(startAddr), NULL);
								if (startAddr >= dwBase && startAddr <= dwEnd) {
									SuspendThread(hThread);
									std::cout << "[!] Frozen AC Thread: " << std::hex << te.th32ThreadID << std::endl;
								}
							}
							CloseHandle(hThread);
						}
					}
				} while (Thread32Next(hSnap, &te));
			}
			CloseHandle(hSnap);
		}
	}

	Sleep(1000);

	std::cout << "[+] BYPASS HAS BEEN ACTIVED..." << std::endl;
	Beep(250, 250);

}

void GameGuard() {

    // ---- Wait for the game window -----------------------------------------
    while (FindWindowA("Rumble Fighter", "Rumble Fighter") == nullptr)
        Sleep(2500);

    uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("RumbleFighter.exe"));

    // ---- Patch 1: main GameGuard check ------------------------------------
    //const char* pattern1 =
    //    "0F 84 ?? ?? ?? ?? 3D 7C 01 00 00 0F 87 ?? ?? ?? ?? 74 ?? "
    //    "8D 48 92 81 F9 FA 00 00 00 77 ?? 0F B6 89 ?? ?? ?? ?? "
    //    "FF 24 8D ?? ?? ?? ?? BE ?? ?? ?? ?? EB";

    //uintptr_t addr1 = scanner::find_pattern(moduleBase, 0x00FFFFFF, pattern1);
    //if (addr1 == 0) {
    //    std::cout << "Didn't find the pattern for bypass!\n";
    //    return;
    //}

    //memapi::write(addr1, "E9 C2 00 00 00 90");   // jmp
    //std::cout << "Bypassed GameGuard Successfully!\n";

    //// NOP out the 10 bytes preceding the patch site
    //memapi::write(addr1 - 10, "90 90 90 90 90 90 90 90 90 90");
    //std::cout << std::hex << (addr1 - 10) << "\n";

    // ---- Patch 2: RF GameHack Detected bypass ------------------------------
    const char* pattern2 =
        "74 77 8B 15 ?? ?? ?? ?? A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? "
        "68 F1 00 00 00 89 85 ?? ?? ?? ?? 89 95 ?? ?? ?? ?? 66 8B 15 "
        "?? ?? ?? ?? 8D 85 ?? ?? ?? ?? 6A 00 50 89 8D ?? ?? ?? ?? "
        "66 89 95 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 C4 0C 6A 00 8D 8D "
        "?? ?? ?? ?? 51 68 ?? ?? ?? ?? 6A 00 FF 15 ?? ?? ?? ?? 6A 01 "
        "E8 ?? ?? ?? ?? 83 C4 04 8B 8E ?? ?? ?? ?? E8 ?? ?? ?? ?? "
        "E9 ?? ?? ?? ?? 8B 57 24";

    uintptr_t addr2 = scanner::find_pattern(moduleBase, 0x00FFFFFF, pattern2);
    memapi::write(addr2, "EB 77");   // je -> jmp

    std::cout << std::hex << addr2 << "\n";

    // ---- Patch 3: ----------------------------------------------------------
    const char* pattern3 =
        "0F 87 ?? ?? ?? ?? 0F B6 80 ?? ?? ?? ?? FF 24 85 ?? ?? ?? ?? "
        "6A 00 E8 ?? ?? ?? ?? 56 8D 8D ?? ?? ?? ?? 68 ?? ?? ?? ?? 51 "
        "E8 ?? ?? ?? ?? 83 C4 0C 6A 00 8D 95 ?? ?? ?? ?? 52 8D 85 ?? "
        "?? ?? ?? 50 6A 00 FF 15 ?? ?? ?? ?? 6A 00 E8 ?? ?? ?? ?? 68 "
        "?? ?? ?? ?? 8D 8D ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 83 C4 08 6A "
        "00 8D 95 ?? ?? ?? ?? 52 8D 85 ?? ?? ?? ?? 50 6A 00 FF 15 ?? "
        "?? ?? ?? 6A 00 E8 ?? ?? ?? ?? 68 ?? ?? ?? ?? EB CB 68 ?? ?? "
        "?? ?? EB C4 8B 15 ?? ?? ?? ?? 8D 8D ?? ?? ?? ?? 51 8B 8A ?? "
        "?? ?? ?? 81 C1 ?? ?? ?? ?? 89 B5 ?? ?? ?? ?? E8 ?? ?? ?? ?? "
        "8B 4D FC";

    uintptr_t addr3 = scanner::find_pattern(moduleBase, 0x00FFFFFF, pattern3);
    memapi::write(addr3, "E9 AF 00 00 00 90");   // jmp

    std::cout << std::hex << addr3 << "\n";

    // ---- Kill GameGuard daemons -------------------------------------------
  //  const std::vector<std::string> processes{ "GameMon.des", "GameMon64.des" };
  //  for (const auto& name : processes)
  //      //utils::unload_dll_by_name(name.c_str());
		//utils::kill_process_by_name(name.c_str());

}