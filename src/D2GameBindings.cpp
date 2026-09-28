#include "D2GameApi.h"

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace d2server100 {

bool D2GameApi100::resolve(const char* moduleName) noexcept {
#if defined(_WIN32)
    if (module) return hasBootstrapSet();
    HMODULE m = GetModuleHandleA(moduleName);
    if (!m) m = LoadLibraryA(moduleName);
    if (!m) return false;
    module = m;
    auto byOrd = [m](unsigned ord) -> FARPROC {
        return GetProcAddress(m, reinterpret_cast<LPCSTR>(MAKEINTRESOURCEA(ord)));
    };
    ord10002 = reinterpret_cast<Ord10002_SetGameData>(byOrd(10002));
    ord10003 = reinterpret_cast<Ord10003_ProcessNetworkMessages>(byOrd(10003));
    ord10007 = reinterpret_cast<Ord10007_DeliverDatabaseCharacter>(byOrd(10007));
    ord10023 = reinterpret_cast<Ord10023_SetServerCallbacks>(byOrd(10023));
    ord10039 = reinterpret_cast<Ord10039_PostGameDataInit>(byOrd(10039));
    ord10040 = reinterpret_cast<Ord10040_ShutdownStage>(byOrd(10040));
    ord10041 = reinterpret_cast<Ord10041_CreateTaskQueue>(byOrd(10041));
    ord10043 = reinterpret_cast<Ord10043_GetDueTask>(byOrd(10043));
    ord10045 = reinterpret_cast<Ord10045_ProcessGameTask>(byOrd(10045));
    ord10046 = reinterpret_cast<Ord10046_RuntimeInit>(byOrd(10046));
    ord10047 = reinterpret_cast<Ord10047_CreateGame>(byOrd(10047));
    ord10048 = reinterpret_cast<Ord10048>(byOrd(10048));
    ord10049 = reinterpret_cast<Ord10049>(byOrd(10049));
    ord10050 = reinterpret_cast<Ord10050_RuntimeShutdown>(byOrd(10050));
    if (!hasBootstrapSet()) { release(); return false; }
    return true;
#else
    (void)moduleName;
    return false;
#endif
}

void D2GameApi100::release() noexcept {
    // The host may already own these modules. Deliberately do not FreeLibrary.
    module=nullptr;
    ord10046=nullptr; ord10023=nullptr; ord10002=nullptr;
    ord10039=nullptr; ord10040=nullptr; ord10050=nullptr;
    ord10003=nullptr; ord10041=nullptr; ord10043=nullptr; ord10045=nullptr;
    ord10007=nullptr; ord10047=nullptr; ord10048=nullptr; ord10049=nullptr;
}

bool D2GameApi100::hasBootstrapSet() const noexcept {
    // v0.4 is an integrated runtime candidate, so fail resolution early if any
    // D2Game export used by bootstrap, MCP control, workers, or teardown is absent.
    return ord10002 && ord10003 && ord10007 && ord10023 &&
           ord10039 && ord10040 && ord10041 && ord10043 && ord10045 &&
           ord10046 && ord10047 && ord10048 && ord10049 && ord10050;
}

} // namespace d2server100
