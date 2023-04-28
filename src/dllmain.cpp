#include "stdafx.h"
#include "helper.hpp"

bool Proxy_Attach();
void Proxy_Detach();

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);

inipp::Ini<char> ini;

// INI Variables
bool bAspectFix;
bool bFOVFix;
bool bCutsceneFPS;
bool bControllerType;
int iControllerType;
int iCustomResX;
int iCustomResY;
int iInjectionDelay;
float fAdditionalFOV; 
int iAspectFix;
int iFOVFix;

// Variables
float fNewX;
float fNewY;
float fNativeAspect = (float)16 / 9;
float fCutsceneAspect = (float)21 / 9;
float fPi = 3.14159265358979323846f;
float fNewAspect;
string sFixVer = "1.0.0";

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

void Logging()
{
    loguru::add_file("JediSurvivorFix.log", loguru::Truncate, loguru::Verbosity_MAX);
    loguru::set_thread_name("Main");

    LOG_F(INFO, "JediSurvivorFix v%s loaded", sFixVer.c_str());
}

void ReadConfig()
{
    // Initialize config
    // UE4 games use launchers so config path is relative to launcher

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
    //inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFOVFix);
    //inipp::get_value(ini.sections["Fix FOV"], "AdditionalFOV", fAdditionalFOV);

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
    //LOG_F(INFO, "Config Parse: bFOVFix: %d", bFOVFix);
    //LOG_F(INFO, "Config Parse: fAdditionalFOV: %.2f", fAdditionalFOV);
    LOG_F(INFO, "Config Parse: iCustomResX: %d", iCustomResX);
    LOG_F(INFO, "Config Parse: iCustomResY: %d", iCustomResY);
    LOG_F(INFO, "Config Parse: fNewX: %.2f", fNewX);
    LOG_F(INFO, "Config Parse: fNewY: %.2f", fNewY);
    LOG_F(INFO, "Config Parse: fNewAspect: %.4f", fNewAspect);
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

            LOG_F(INFO, "Aspect Ratio/FOV: Hook length is %d bytes", AspectFOVFixHookLength);
            LOG_F(INFO, "Aspect Ratio/FOV: Hook address is 0x%" PRIxPTR, (uintptr_t)AspectFOVFixAddress);
        }
        else if (!AspectFOVFixScanResult)
        {
            LOG_F(INFO, "Aspect Ratio/FOV: Pattern scan failed.");
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

