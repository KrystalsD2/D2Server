#pragma once
#include "Compat.h"

namespace d2server100 {

// Exact global engine imports used by retail D2Server.dll 1.00 around
// WinMain 0x10009F35 and 0x1000A159.  Names stay intentionally conservative;
// the binary proves the ordinals, call ABI and lifecycle order.
struct D2CommonApi100 {
    // Retail D2Server calls #10554 with (0,1,0); target returns with ret 0x0C.
    using Ord10554_InitializeDataTables = void (D2_STDCALL *)(u32 arg0, u32 arg1, u32 arg2);
    using Ord10553_ShutdownDataTables   = void (D2_CDECL *)();
    // Retail D2Server DEBUG.LOG initializer jumps to D2Common #10980 with ECX=1.
    using Ord10980_SetDebugLogRaw       = void (D2_CDECL *)();
    // D2Common\Logging\ProfCore.cpp final profiling report, not a destructor.
    using Ord10983_DumpProfiler         = void (D2_CDECL *)();

    void* module = nullptr;
    Ord10554_InitializeDataTables ord10554 = nullptr;
    Ord10553_ShutdownDataTables ord10553 = nullptr;
    Ord10980_SetDebugLogRaw ord10980 = nullptr;
    Ord10983_DumpProfiler ord10983 = nullptr;

    bool resolve(const char* moduleName = "D2Common.dll") noexcept;
    void release() noexcept;
    bool ready() const noexcept { return ord10554 && ord10553 && ord10983; }
};

struct D2LangApi100 {
    // Retail explicitly places zero in ECX before #10000.  The export has no
    // stack cleanup, so model the observed x86 register ABI directly.
    using Ord10000_Initialize = int  (D2_FASTCALL *)(u32 contextZero);
    using Ord10001_Shutdown   = void (D2_CDECL *)();

    void* module = nullptr;
    Ord10000_Initialize ord10000 = nullptr;
    Ord10001_Shutdown ord10001 = nullptr;

    bool resolve(const char* moduleName = "D2Lang.dll") noexcept;
    void release() noexcept;
    bool ready() const noexcept { return ord10000 && ord10001; }
};

D2CommonApi100& d2commonApi100() noexcept;
D2LangApi100& d2langApi100() noexcept;

// Exact retail DEBUG.LOG side effect; #10980 receives the enable value in ECX.
bool SetD2CommonDebugLog100(bool enabled) noexcept;

// Retail order:
//   D2Lang #10000(ECX=0) -> require nonzero
//   D2Common #10554(0,1,0)
// ... service runtime ...
//   D2Common #10553 -> D2Lang #10001 -> D2Common #10983 profiler dump
bool InitializeEngineGlobals100() noexcept;
void ShutdownEngineGlobals100() noexcept;
bool EngineGlobalsInitialized100() noexcept;

} // namespace d2server100
