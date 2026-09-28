#include "D2NetApi100.h"

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace d2server100 {

D2NetApi100& d2netApi100() noexcept {
    static D2NetApi100 api{};
    return api;
}

bool D2NetApi100::resolve(const char* moduleName) noexcept {
#if defined(_WIN32)
    if (module) return ready();
    HMODULE m = GetModuleHandleA(moduleName);
    if (!m) m = LoadLibraryA(moduleName);
    if (!m) return false;
    module = m;
    auto byOrd = [m](unsigned ord) -> FARPROC {
        return GetProcAddress(m, reinterpret_cast<LPCSTR>(MAKEINTRESOURCEA(ord)));
    };
    ord10003 = reinterpret_cast<Ord10003_ServerInitialize>(byOrd(10003));
    ord10004 = reinterpret_cast<Ord10004_SendShutdownControl>(byOrd(10004));
    ord10018 = reinterpret_cast<Ord10018_SetHackCallback>(byOrd(10018));
    ord10022 = reinterpret_cast<Ord10022_WaitAndPump>(byOrd(10022));
    ord10023 = reinterpret_cast<Ord10023_SetInternalModeFlag>(byOrd(10023));
    ord10026 = reinterpret_cast<Ord10026_SetMultiClientLimit>(byOrd(10026));
    if (!ready()) { release(); return false; }
    return true;
#else
    (void)moduleName;
    return false;
#endif
}

void D2NetApi100::release() noexcept {
    module = nullptr;
    ord10003 = nullptr;
    ord10004 = nullptr;
    ord10018 = nullptr;
    ord10022 = nullptr;
    ord10023 = nullptr;
    ord10026 = nullptr;
}

} // namespace d2server100
