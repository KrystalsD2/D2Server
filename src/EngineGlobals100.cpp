#include "EngineGlobals100.h"

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace d2server100 {
namespace {
bool g_languageInitialized = false;
bool g_commonInitialized = false;
}

D2CommonApi100& d2commonApi100() noexcept {
    static D2CommonApi100 api{};
    return api;
}
D2LangApi100& d2langApi100() noexcept {
    static D2LangApi100 api{};
    return api;
}

bool D2CommonApi100::resolve(const char* moduleName) noexcept {
#if defined(_WIN32)
    if (module) return ready();
    HMODULE m = GetModuleHandleA(moduleName);
    if (!m) m = LoadLibraryA(moduleName);
    if (!m) return false;
    module = m;
    auto byOrd = [m](unsigned ord) -> FARPROC {
        return GetProcAddress(m, reinterpret_cast<LPCSTR>(MAKEINTRESOURCEA(ord)));
    };
    ord10554 = reinterpret_cast<Ord10554_InitializeDataTables>(byOrd(10554));
    ord10553 = reinterpret_cast<Ord10553_ShutdownDataTables>(byOrd(10553));
    ord10980 = reinterpret_cast<Ord10980_SetDebugLogRaw>(byOrd(10980));
    ord10983 = reinterpret_cast<Ord10983_DumpProfiler>(byOrd(10983));
    if (!ready()) { release(); return false; }
    return true;
#else
    (void)moduleName;
    return false;
#endif
}

void D2CommonApi100::release() noexcept {
    module = nullptr;
    ord10554 = nullptr;
    ord10553 = nullptr;
    ord10980 = nullptr;
    ord10983 = nullptr;
}

bool D2LangApi100::resolve(const char* moduleName) noexcept {
#if defined(_WIN32)
    if (module) return ready();
    HMODULE m = GetModuleHandleA(moduleName);
    if (!m) m = LoadLibraryA(moduleName);
    if (!m) return false;
    module = m;
    auto byOrd = [m](unsigned ord) -> FARPROC {
        return GetProcAddress(m, reinterpret_cast<LPCSTR>(MAKEINTRESOURCEA(ord)));
    };
    ord10000 = reinterpret_cast<Ord10000_Initialize>(byOrd(10000));
    ord10001 = reinterpret_cast<Ord10001_Shutdown>(byOrd(10001));
    if (!ready()) { release(); return false; }
    return true;
#else
    (void)moduleName;
    return false;
#endif
}

void D2LangApi100::release() noexcept {
    module = nullptr;
    ord10000 = nullptr;
    ord10001 = nullptr;
}

bool SetD2CommonDebugLog100(bool enabled) noexcept {
    auto& common=d2commonApi100();
    if (!common.module && !common.resolve()) return false;
    if (!common.ord10980) return false;
#if defined(_MSC_VER) && defined(_M_IX86)
    auto fn=common.ord10980;
    const u32 value=enabled ? 1u : 0u;
    __asm {
        mov ecx, value
        call fn
    }
    return true;
#else
    // #10980 is a register-argument export. Portable verification locks the
    // state decision but deliberately does not call it with a fake stack ABI.
    (void)enabled;
    return true;
#endif
}

bool InitializeEngineGlobals100() noexcept {
    if (g_languageInitialized && g_commonInitialized) return true;

    auto& lang = d2langApi100();
    auto& common = d2commonApi100();
    if (!lang.ready() && !lang.resolve()) return false;
    if (!common.ready() && !common.resolve()) return false;

    // Retail D2Server helper 0x10001810 returns zero; WinMain moves that zero
    // into ECX before calling D2Lang #10000 and aborts startup on a zero return.
    if (!g_languageInitialized) {
        if (!lang.ord10000(0)) return false;
        g_languageInitialized = true;
    }

    if (!g_commonInitialized) {
        // Exact pushes at 0x10009F49..0x10009F53 produce logical args (0,1,0).
        common.ord10554(0,1,0);
        g_commonInitialized = true;
    }
    return true;
}

void ShutdownEngineGlobals100() noexcept {
    auto& common = d2commonApi100();
    auto& lang = d2langApi100();

    // Exact retail order at 0x1000A159..0x1000A168.  #10983 is retained in
    // its real role as the ProfCore final dump after the data/lang teardown.
    if (g_commonInitialized && common.ord10553) common.ord10553();
    if (g_languageInitialized && lang.ord10001) lang.ord10001();
    if (g_commonInitialized && common.ord10983) common.ord10983();

    g_commonInitialized = false;
    g_languageInitialized = false;
}

bool EngineGlobalsInitialized100() noexcept {
    return g_languageInitialized && g_commonInitialized;
}

} // namespace d2server100
