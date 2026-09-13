#include <Windows.h>
#include "memory.h"
#include "scanner.h"
#include "resolver.h"

#define dw_start  0x00400000
#define dw_end    0x00FFFFFF


uintptr_t KillAll = 0;
uintptr_t Hit = 0;
uintptr_t MonstersKO = 0;
uintptr_t BossKO = 0;
uintptr_t KillMonsterFunc = 0;
uintptr_t KillBossFunc = 0;

void resolveAddresses()
{

    const char* KillAll_pattern = "75 08 85 FF 0F 84 35 02 00 00";
    KillAll = scanner::find_pattern(dw_start, dw_end, KillAll_pattern, 1);

    const char* Hit_pattern = "56 51 8B 4D F8";
    Hit = scanner::find_pattern(dw_start, dw_end, Hit_pattern, 1);

    const char* MonstersKO_pattern = "0F 8E ?? ?? ?? ?? 53 E8 ?? ?? ?? ?? 8B C3 83 E0 03 83 F8 03 77 ?? FF 24 85 ?? ?? ?? ?? 0F B7 47 28 8B 8E 88 00 00 00 6A 00";
    MonstersKO = scanner::find_pattern(dw_start, dw_end, MonstersKO_pattern, 2);

    const char* BossKO_pattern = "0F 8E ?? ?? ?? ?? 53 E8 ?? ?? ?? ?? 8B C3 83 E0 03 83 F8 03 0F 87 ?? ?? ?? ?? FF 24 85 ?? ?? ?? ?? 0F B7 47 28 8B 8E 88 00 00 00 6A 00";
    BossKO = scanner::find_pattern(dw_start, dw_end, BossKO_pattern, 1);

    const char* KillMonsterFunc_Pattern = "03 00 00 8B C8 0F B6 51 04";
	KillMonsterFunc = scanner::find_pattern(dw_start, dw_end, KillMonsterFunc_Pattern, 1) + 3; // We add 3 to point to the start of the function after the instruction prefix.
    
    const char* KillBossFunc_Pattern = "01 00 00 8B C8 0F B6 51 04";
    KillBossFunc = scanner::find_pattern(dw_start, dw_end, KillBossFunc_Pattern, 1) + 3; // We add 3 to point to the start of the function after the instruction prefix.
    

    if (KillAll == 0 || KillAll == dw_start)
    {
        MessageBoxA(
            nullptr,
            "[-]Resolver has failed, please contact P3tWu33y.",
            "Resolver Error",
            MB_OK | MB_ICONERROR
        );

        return;
    }

    if (Hit == 0 || Hit == dw_start)
    {
        MessageBoxA(
            nullptr,
            "[-]Resolver has failed, please contact P3tWu33y.",
            "Resolver Error",
            MB_OK | MB_ICONERROR
        );

        return;
    }

    if (MonstersKO == 0 || MonstersKO == dw_start)
    {
        MessageBoxA(
            nullptr,
            "[-]Resolver has failed, please contact P3tWu33y.",
            "Resolver Error",
            MB_OK | MB_ICONERROR
        );

        return;
    }

    if (BossKO == 0 || BossKO == dw_start)
    {
        MessageBoxA(
            nullptr,
            "[-]Resolver has failed, please contact P3tWu33y.",
            "Resolver Error",
            MB_OK | MB_ICONERROR
        );

        return;
    }

    if (KillMonsterFunc == 0 || KillMonsterFunc == dw_start)
    {
        MessageBoxA(
            nullptr,
            "[-]Resolver has failed, please contact P3tWu33y.",
            "Resolver Error",
            MB_OK | MB_ICONERROR
        );

        return;
    }

    if (KillBossFunc == 0 || KillBossFunc == dw_start)
    {
        MessageBoxA(
            nullptr,
            "[-]Resolver has failed, please contact P3tWu33y.",
            "Resolver Error",
            MB_OK | MB_ICONERROR
        );

        return;
    }


	// Debug output to console -- Comment it when you are done testing.
	std::cout << "[+]BossKO: 0x" << std::hex << BossKO << std::endl;
    std::cout << "[+]MonstersKO: 0x" << std::hex << MonstersKO << std::endl;
    std::cout << "[+]Hit: 0x" << std::hex << Hit << std::endl;
    std::cout << "[+]KillAll: 0x" << std::hex << KillAll << std::endl;
	std::cout << "[+]KillMonsterFunc Function: 0x" << std::hex << KillMonsterFunc << std::endl;
    std::cout << "[+]KillBossFunc Function: 0x" << std::hex << KillBossFunc << std::endl;
}



