// dllmain.cpp : Defines the entry point for the DLL application.
#include "SlideFix.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    static SlideFix* _SlideFix = NULL;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        _SlideFix = new SlideFix();
        _SlideFix-> Initialize();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return true;
}