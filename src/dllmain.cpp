#include "stdafx.h"
#include "helper.hpp"

bool Proxy_Attach();
void Proxy_Detach();

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);

inipp::Ini<char> ini;

// INI Variables
bool bAspectFix;
bool bGameplayCamera;
int iCustomResX;
int iCustomResY;
int iInjectionDelay;
float fFOVMulti = (float)1; 
float fCameraDistanceMulti = (float)1;
int iAspectFix;
int iFOVFix;

// Variables
float fNewX;
float fNewY;
float fNativeAspect = (float)16 / 9;
float fCutsceneAspect = (float)21 / 9;
float fPi = 3.14159265358979323846f;
float fOne = (float)1;
float fNewAspect;
string sFixVer = "1.0.1";

// Aspect Ratio/FOV Hook
DWORD64 AspectFOVFixReturnJMP;
void __declspec(naked) AspectFOVFix_CC()
{
    __asm
    {
        mov eax, [rbx + 0x000002BC]             // Original code
        mov[rdi + 0x30], eax                    // Original code
        movzx eax, byte ptr[rbx + 0x000002C8]   // Original code
        mov eax, 0                              // Do not constrain aspect ratio
        jmp [AspectFOVFixReturnJMP]                   
    }
}

// Gameplay FOV Hook
DWORD64 GameplayFOVReturnJMP;
void __declspec(naked) GameplayFOV_CC()
{
    __asm
    {
        movss xmm0, [fFOVMulti]
        subss xmm0, [fOne]
        movaps xmm6, xmm0                           // Original code
        //minss xmm6, [JediSurvivor.exe + 4B60C0C]  // Original code
        movaps xmm7, xmm6                           // Original code
        jmp[GameplayFOVReturnJMP]
    }
}

// Gameplay Camera Distance Hook
DWORD64 GameplayCameraDistanceReturnJMP;
void __declspec(naked) GameplayCameraDistance_CC()
{
    __asm
    {
        movss xmm6, [fCameraDistanceMulti]
        //minss xmm6, [JediSurvivor.exe + 4B60C10] // Original Code
        movss xmm7, [fOne]      
        jmp[GameplayCameraDistanceReturnJMP]
    }
}

void Logging()
{
    loguru::add_file("JediSurvivorFix.log", loguru::Truncate, loguru::Verbosity_MAX);
    loguru::set_thread_name("Main");

    LOG_F(INFO, "JediSurvivorFix v%s loaded", sFixVer.c_str());
}

void ReadConfig()
{
    // Initialize config
    // This game has no launcher despite being UE4
    std::ifstream iniFile(".\\JediSurvivorFix.ini");
    if (!iniFile)
    {
        LOG_F(ERROR, "Failed to load config file.");
    }
    else
    {
        ini.parse(iniFile);
    }

    inipp::get_value(ini.sections["JediSurvivorFix Parameters"], "InjectionDelay", iInjectionDelay);
    inipp::get_value(ini.sections["Disable Pillarboxing"], "Enabled", bAspectFix);
    inipp::get_value(ini.sections["Gameplay Camera"], "Enabled", bGameplayCamera);
    inipp::get_value(ini.sections["Gameplay Camera"], "FOVMultiplier", fFOVMulti);
    fFOVMulti = std::clamp(fFOVMulti, (float)0, (float)10);
    inipp::get_value(ini.sections["Gameplay Camera"], "CameraDistance", fCameraDistanceMulti);
    fCameraDistanceMulti = std::clamp(fCameraDistanceMulti, (float)0, (float)10);

    // Custom resolution
    if (iCustomResX > 0 && iCustomResY > 0)
    {
        fNewX = (float)iCustomResX;
        fNewY = (float)iCustomResY;
        fNewAspect = (float)iCustomResX / (float)iCustomResY;
    }
    else
    {
        // Grab desktop resolution
        RECT desktop;
        GetWindowRect(GetDesktopWindow(), &desktop);
        fNewX = (float)desktop.right;
        fNewY = (float)desktop.bottom;
        iCustomResX = (int)desktop.right;
        iCustomResY = (int)desktop.bottom;
        fNewAspect = (float)desktop.right / (float)desktop.bottom;
    }

    // Log config parse
    LOG_F(INFO, "Config Parse: iInjectionDelay: %dms", iInjectionDelay);
    LOG_F(INFO, "Config Parse: bAspectFix: %d", bAspectFix);
    LOG_F(INFO, "Config Parse: bGameplayCamera: %d", bGameplayCamera);
    LOG_F(INFO, "Config Parse: fFOVMulti: %.2f", fFOVMulti);
    LOG_F(INFO, "Config Parse: fCameraDistanceMulti: %.2f", fCameraDistanceMulti);
    //LOG_F(INFO, "Config Parse: iCustomResX: %d", iCustomResX);
    //LOG_F(INFO, "Config Parse: iCustomResY: %d", iCustomResY);
    //LOG_F(INFO, "Config Parse: fNewX: %.2f", fNewX);
    //LOG_F(INFO, "Config Parse: fNewY: %.2f", fNewY);
    //LOG_F(INFO, "Config Parse: fNewAspect: %.4f", fNewAspect);
}

void AspectFOVFix()
{
    if (bAspectFix)
    {
        uint8_t* AspectFOVFixScanResult = Memory::PatternScan(baseModule, "EB ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? 8B ?? ?? ?? ?? ?? 89 ?? ?? 8B ?? ?? ?? ?? ??");
        if (AspectFOVFixScanResult)
        {
            DWORD64 AspectFOVFixAddress = (uintptr_t)AspectFOVFixScanResult + 0x18;
            int AspectFOVFixHookLength = Memory::GetHookLength((char*)AspectFOVFixAddress, 13);
            AspectFOVFixReturnJMP = AspectFOVFixAddress + AspectFOVFixHookLength;
            Memory::DetourFunction64((void*)AspectFOVFixAddress, AspectFOVFix_CC, AspectFOVFixHookLength);

            LOG_F(INFO, "Aspect Ratio: Hook length is %d bytes", AspectFOVFixHookLength);
            LOG_F(INFO, "Aspect Ratio: Hook address is 0x%" PRIxPTR, (uintptr_t)AspectFOVFixAddress);
        }
        else if (!AspectFOVFixScanResult)
        {
            LOG_F(INFO, "Aspect Ratio: Pattern scan failed.");
        }
    }

    if (bGameplayCamera)
    {
        uint8_t* GameplayFOVScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? 0F ?? ?? 72 ?? 0F ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? ?? ?? ?? ?? 76 ??");
        if (GameplayFOVScanResult)
        {
            DWORD64 GameplayFOVAddress = (uintptr_t)GameplayFOVScanResult + 0x9;
            int GameplayFOVHookLength = Memory::GetHookLength((char*)GameplayFOVAddress, 13);
            GameplayFOVReturnJMP = GameplayFOVAddress + GameplayFOVHookLength;
            Memory::DetourFunction64((void*)GameplayFOVAddress, GameplayFOV_CC, GameplayFOVHookLength);

            LOG_F(INFO, "Gameplay FOV: Hook length is %d bytes", GameplayFOVHookLength);
            LOG_F(INFO, "Gameplay FOV: Hook address is 0x%" PRIxPTR, (uintptr_t)GameplayFOVAddress);
        }
        else if (!GameplayFOVScanResult)
        {
            LOG_F(INFO, "Gameplay FOV: Pattern scan failed.");
        }

        uint8_t* GameplayCameraDistanceScanResult = Memory::PatternScan(baseModule, "0F ?? ?? 72 ?? 0F ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? F3 0F ?? ?? 0F ");
        if (GameplayCameraDistanceScanResult)
        {
            DWORD64 GameplayCameraDistanceAddress = (uintptr_t)GameplayCameraDistanceScanResult + 0x8;
            int GameplayCameraDistanceHookLength = Memory::GetHookLength((char*)GameplayCameraDistanceAddress, 13);
            GameplayCameraDistanceReturnJMP = GameplayCameraDistanceAddress + GameplayCameraDistanceHookLength;
            Memory::DetourFunction64((void*)GameplayCameraDistanceAddress, GameplayCameraDistance_CC, GameplayCameraDistanceHookLength);

            LOG_F(INFO, "Gameplay Camera Distance: Hook length is %d bytes", GameplayCameraDistanceHookLength);
            LOG_F(INFO, "Gameplay Camera Distance: Hook address is 0x%" PRIxPTR, (uintptr_t)GameplayCameraDistanceAddress);
        }
        else if (!GameplayCameraDistanceScanResult)
        {
            LOG_F(INFO, "Gameplay Camera Distance: Pattern scan failed.");
        }
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    Sleep(iInjectionDelay);
    AspectFOVFix();
    return true; // end thread
}

HMODULE ourModule;

void Patch_Uninit()
{

}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        ourModule = hModule;
        Proxy_Attach();

        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);

        if (mainHandle)
        {
            CloseHandle(mainHandle);
        }
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

