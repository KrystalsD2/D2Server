#include "StormApi100.h"

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace d2server100 {

StormApi100& stormApi100() noexcept {
    static StormApi100 api{};
    return api;
}

bool StormApi100::resolve(const char* moduleName) noexcept {
#if defined(_WIN32)
    if (module) return ready();
    HMODULE m = GetModuleHandleA(moduleName);
    if (!m) m = LoadLibraryA(moduleName);
    if (!m) return false;
    module = m;
    auto byOrd = [m](unsigned ord) -> FARPROC {
        return GetProcAddress(m, reinterpret_cast<LPCSTR>(MAKEINTRESOURCEA(ord)));
    };
    alloc401 = reinterpret_cast<AllocFn>(byOrd(401));
    free403  = reinterpret_cast<FreeFn>(byOrd(403));
    if (!ready()) { release(); return false; }
    return true;
#else
    (void)moduleName;
    return false;
#endif
}

void StormApi100::release() noexcept {
    // Like the D2Game resolver, this may only borrow a module already loaded by
    // the host; deliberately do not FreeLibrary it here.
    module = nullptr;
    alloc401 = nullptr;
    free403 = nullptr;
}

} // namespace d2server100
