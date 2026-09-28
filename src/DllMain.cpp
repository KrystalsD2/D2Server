#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static HINSTANCE g_hInstance = nullptr;
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) g_hInstance = hinst;
    return TRUE;
}
#else
// Non-Windows analysis builds intentionally omit DllMain.
#endif
