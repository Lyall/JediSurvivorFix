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
float fNativeAspect = (float)16/9;
float fPi = 3.14159265358979323846f;
float fNewAspect;
int iNarrowAspect;
string sFixVer = "1.0.0";

// CurrResolution Hook
DWORD64 CurrResolutionReturnJMP;
void __declspec(naked) CurrResolution_CC()
{
    __asm
    {
        mov[rsi + 0x000000A0], edx              // Original Code
        mov[rsi + 0x000000A4], eax              // Original Code
        mov[rsi + 0x000000A8], r9d              // Original Code

        mov[iCustomResX], r9d                 // Grab current resX
        mov[iCustomResY], ecx                 // Grab current resY
        cvtsi2ss xmm14, r9d
        cvtsi2ss xmm15, ecx
        divss xmm14, xmm15
        movss[fNewAspect], xmm14              // Grab current aspect ratio
        movss xmm15, [fNativeAspect]
        comiss xmm14, xmm15
        ja narrowAspect
        mov[iNarrowAspect], 1
        xorps xmm14, xmm14
        xorps xmm15, xmm15
        jmp[CurrResolutionReturnJMP]

        narrowAspect:
            mov[iNarrowAspect], 0
            xorps xmm14, xmm14
            xorps xmm15, xmm15
            jmp[CurrResolutionReturnJMP]
    }
}

// Aspect Ratio/FOV Hook
DWORD64 AspectFOVFixReturnJMP;
float FOVPiDiv;
float FOVDivPi;
float FOVFinalValue;
void __declspec(naked) AspectFOVFix_CC()
{
    __asm
    {
        cmp [iNarrowAspect], 1
        je originalCode                       // Skip FOV fix if fNewAspect<=fNativeAspect
        cmp [iFOVFix], 1                       // Check if FOVFix is enabled
        je modifyFOV                           // jmp to FOV fix
        jmp originalCode                       // jmp to originalCode

        modifyFOV:
            fld dword ptr[rbx + 0x2A8]         // Push original FOV to FPU register st(0)
            fmul[FOVPiDiv]                     // Multiply st(0) by Pi/360
            fptan                              // Get partial tangent. Store result in st(1). Store 1.0 in st(0)
            fxch st(1)                         // Swap st(1) to st(0)
            fdiv[fNativeAspect]                // Divide st(0) by 1.778~
            fmul[fNewAspect]                   // Multiply st(0) by new aspect ratio
            fxch st(1)                         // Swap st(1) to st(0)
            fpatan                             // Get partial arc tangent from st(0), st(1)
            fmul[FOVDivPi]                     // Multiply st(0) by 360/Pi
            fadd[fAdditionalFOV]               // Add additional FOV
            fstp[FOVFinalValue]                // Store st(0) 
            movss xmm0, [FOVFinalValue]        // Copy final FOV value to xmm0
            jmp originalCode

        originalCode:
            movss[rdi + 0x18], xmm0            // Original code
            cmp [iAspectFix], 1
            je modifyAspect
            mov eax, [rbx + 0x000002B8]        // Original code
            mov[rdi + 0x2C], eax               // Original code
            jmp [AspectFOVFixReturnJMP]

        modifyAspect:
            mov eax, [fNewAspect]
            mov[rdi + 0x2C], eax               // Original code
            jmp[AspectFOVFixReturnJMP]                         
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
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bAspectFix);
    iAspectFix = (int)bAspectFix;
    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFOVFix);
    iFOVFix = (int)bFOVFix;
    inipp::get_value(ini.sections["Fix FOV"], "AdditionalFOV", fAdditionalFOV);

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
    LOG_F(INFO, "Config Parse: bFOVFix: %d", bFOVFix);
    LOG_F(INFO, "Config Parse: fAdditionalFOV: %.2f", fAdditionalFOV);
    LOG_F(INFO, "Config Parse: iCustomResX: %d", iCustomResX);
    LOG_F(INFO, "Config Parse: iCustomResY: %d", iCustomResY);
    LOG_F(INFO, "Config Parse: fNewX: %.2f", fNewX);
    LOG_F(INFO, "Config Parse: fNewY: %.2f", fNewY);
    LOG_F(INFO, "Config Parse: fNewAspect: %.4f", fNewAspect);
}

void AspectFOVFix()
{
    if (bAspectFix || bFOVFix)
    {
        uint8_t* CurrResolutionScanResult = Memory::PatternScan(baseModule, "89 ?? ?? ?? ?? ?? 89 ?? ?? ?? ?? ?? 44 ?? ?? ?? ?? ?? ?? 89 ?? ?? ?? 44 ?? ?? ??");
        if (CurrResolutionScanResult)
        {
            DWORD64 CurrResolutionAddress = (uintptr_t)CurrResolutionScanResult;
            int CurrResolutionHookLength = Memory::GetHookLength((char*)CurrResolutionAddress, 13);
            CurrResolutionReturnJMP = CurrResolutionAddress + CurrResolutionHookLength;
            Memory::DetourFunction64((void*)CurrResolutionAddress, CurrResolution_CC, CurrResolutionHookLength);

            LOG_F(INFO, "Current Resolution: Hook length is %d bytes", CurrResolutionHookLength);
            LOG_F(INFO, "Current Resolution: Hook address is 0x%" PRIxPTR, (uintptr_t)CurrResolutionAddress);
        }
        else if (!CurrResolutionScanResult)
        {
            LOG_F(INFO, "Current Resolution: Pattern scan failed.");
        }

        uint8_t* AspectFOVFixScanResult = Memory::PatternScan(baseModule, "EB ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? 8B ?? ?? ?? ?? ?? 89 ?? ?? 8B ?? ?? ?? ?? ??");
        if (AspectFOVFixScanResult)
        {
            FOVPiDiv = fPi / 360;
            FOVDivPi = 360 / fPi;

            DWORD64 AspectFOVFixAddress = (uintptr_t)AspectFOVFixScanResult + 0xA;
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

