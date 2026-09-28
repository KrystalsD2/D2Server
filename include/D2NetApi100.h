#pragma once
#include "Compat.h"

namespace d2server100 {

// Exact Diablo II 1.00 D2Net exports used by the May-26-2000 D2Server.dll.
// Names describe only behavior that is directly evidenced by the binaries.
struct D2NetApi100 {
    // Fog invokes the callback stored by D2Net #10018 with the 32-bit value in
    // ECX.  On x86 this is therefore a one-argument __fastcall callback, not
    // an ordinary stack/stdcall callback.
    using HackReportCallback = void (D2_FASTCALL *)(u32 ipv4OrKey);

    // #10003 is a two-DWORD setup wrapper.  Retail D2Server passes (0,0);
    // D2Net itself supplies protocol/type 2 and TCP port 4000 to Fog.  Keep
    // argument names neutral until their inner semantic meaning is proven.
    using Ord10003_ServerInitialize     = int  (D2_STDCALL *)(u32 arg0, u32 arg1);
    using Ord10004_SendShutdownControl = void (D2_CDECL *)();
    using Ord10018_SetHackCallback     = void (D2_STDCALL *)(HackReportCallback callback);
    using Ord10022_WaitAndPump         = int  (D2_STDCALL *)(u32 waitMs);
    using Ord10023_SetInternalModeFlag = void (D2_STDCALL *)(u32 value);
    using Ord10026_SetMultiClientLimit = void (D2_STDCALL *)(u32 value);

    void* module = nullptr;
    Ord10003_ServerInitialize ord10003 = nullptr;
    Ord10004_SendShutdownControl ord10004 = nullptr;
    Ord10018_SetHackCallback ord10018 = nullptr;
    Ord10022_WaitAndPump ord10022 = nullptr;
    Ord10023_SetInternalModeFlag ord10023 = nullptr;
    Ord10026_SetMultiClientLimit ord10026 = nullptr;

    bool resolve(const char* moduleName = "D2Net.dll") noexcept;
    void release() noexcept;
    bool ready() const noexcept {
        return ord10003 && ord10004 && ord10018 && ord10022 && ord10023 && ord10026;
    }
};

D2NetApi100& d2netApi100() noexcept;

} // namespace d2server100
