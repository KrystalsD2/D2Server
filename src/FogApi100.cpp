#include "FogApi100.h"

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace d2server100 {

FogApi100& fogApi100() noexcept { static FogApi100 api{}; return api; }

bool FogApi100::resolve(const char* moduleName) noexcept {
#if defined(_WIN32)
    if (module) return ready();
    HMODULE m=GetModuleHandleA(moduleName);
    if (!m) m=LoadLibraryA(moduleName);
    if (!m) return false;
    module=m;
    auto byOrd=[m](unsigned ord)->FARPROC {
        return GetProcAddress(m,reinterpret_cast<LPCSTR>(MAKEINTRESOURCEA(ord)));
    };
    ord10139=reinterpret_cast<Ord10139_SetGlobalMode>(byOrd(10139));
    ord10019=reinterpret_cast<Ord10019_InitializeErrorManager>(byOrd(10019));
    ord10020=reinterpret_cast<Ord10020_GetSystemDescriptor>(byOrd(10020));
    ord10036=reinterpret_cast<Ord10036Fn>(byOrd(10036));
    ord10047=reinterpret_cast<Ord10047_CreateClient>(byOrd(10047));
    ord10048=reinterpret_cast<Ord10048_DestroyClient>(byOrd(10048));
    ord10049=reinterpret_cast<Ord10049_SendFramed>(byOrd(10049));
    ord10050=reinterpret_cast<Ord10050_SendControl>(byOrd(10050));
    ord10051=reinterpret_cast<Ord10051_Receive>(byOrd(10051));
    ord10052=reinterpret_cast<Ord10052_StartBlocking>(byOrd(10052));
    ord10053=reinterpret_cast<Ord10053_StartAsync>(byOrd(10053));
    ord10056=reinterpret_cast<Ord10056_Configure>(byOrd(10056));
    ord10058=reinterpret_cast<Ord10058_ActiveTransport>(byOrd(10058));
    if (!ready()) { release(); return false; }
    return true;
#else
    (void)moduleName;
    return false;
#endif
}

void FogApi100::release() noexcept {
    module=nullptr;
    ord10139=nullptr;
    ord10019=nullptr;
    ord10020=nullptr;
    ord10036=nullptr;
    ord10047=nullptr; ord10048=nullptr; ord10049=nullptr; ord10050=nullptr;
    ord10051=nullptr; ord10052=nullptr; ord10053=nullptr; ord10056=nullptr; ord10058=nullptr;
}

} // namespace d2server100
