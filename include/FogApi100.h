#pragma once
#include "Compat.h"

namespace d2server100 {

struct FogApi100 {
    // First Fog call in retail D2Server WinMain: ECX=1 -> Fog #10139. The
    // May-2000 implementation is a one-instruction global-mode setter used by
    // Fog allocation/helper paths to select their server-context behavior.
    using Ord10139_SetGlobalMode = void (D2_FASTCALL *)(u32 mode);

    // Retail D2Server calls Fog #10019 once with ECX = "D2SDebug" before
    // reading realms.ini.  The target is Fog\Src\ErrorManager.cpp and builds
    // the application diagnostic/log path from that stem.
    using Ord10019_InitializeErrorManager = void (D2_FASTCALL *)(const char* logStem);

    // #10020 returns a pointer to Fog's process/system descriptor. The
    // SYSTEM_INFO sub-structure begins at +0x14, so +0x28 is exactly
    // SYSTEM_INFO::dwNumberOfProcessors; stock D2Server uses that as its game
    // worker thread count.
    using Ord10020_GetSystemDescriptor = const void* (D2_CDECL *)();
    using Ord10036Fn = u32 (D2_CDECL *)();

    // Fog #10056 stores this callback in the client object.  Its receive path
    // calls it with packet in ECX, length in EDX and context as the one stack
    // argument.  MSVC x86 __fastcall therefore reproduces the observed ABI.
    using ClientPacketCallback = int (D2_FASTCALL *)(const u8* packet, u32 length, void* context);

    // Network-client object used by original D2Server for MCP (:6112)
    // and Character Server (:6113) links.
    using Ord10047_CreateClient = void* (D2_STDCALL *)(const char* address, u32 port, const char* identity);
    using Ord10048_DestroyClient = void (D2_STDCALL *)(void* connection);
    using Ord10049_SendFramed = int (D2_STDCALL *)(void* connection, const void* data, u32 length);
    using Ord10050_SendControl = int (D2_STDCALL *)(void* connection, const void* data, u32 length);
    using Ord10051_Receive = int (D2_STDCALL *)(void* connection, void* buffer, u32 capacity);
    using Ord10052_StartBlocking = void (D2_STDCALL *)(void* connection);
    using Ord10053_StartAsync = void* (D2_STDCALL *)(void* connection);
    using Ord10056_Configure = void (D2_STDCALL *)(void* connection, ClientPacketCallback callback, void* context);
    using Ord10058_ActiveTransport = void* (D2_STDCALL *)(void* connection);

    void* module = nullptr;
    Ord10139_SetGlobalMode ord10139 = nullptr;
    Ord10019_InitializeErrorManager ord10019 = nullptr;
    Ord10020_GetSystemDescriptor ord10020 = nullptr;
    Ord10036Fn ord10036 = nullptr;
    Ord10047_CreateClient ord10047 = nullptr;
    Ord10048_DestroyClient ord10048 = nullptr;
    Ord10049_SendFramed ord10049 = nullptr;
    Ord10050_SendControl ord10050 = nullptr;
    Ord10051_Receive ord10051 = nullptr;
    Ord10052_StartBlocking ord10052 = nullptr;
    Ord10053_StartAsync ord10053 = nullptr;
    Ord10056_Configure ord10056 = nullptr;
    Ord10058_ActiveTransport ord10058 = nullptr;

    bool resolve(const char* moduleName = "Fog.dll") noexcept;
    void release() noexcept;
    bool ready() const noexcept { return ord10036 != nullptr; }
    bool transportReady() const noexcept {
        return ord10047 && ord10048 && ord10049 && ord10050 && ord10051 &&
               ord10052 && ord10053 && ord10056 && ord10058;
    }
};

FogApi100& fogApi100() noexcept;

} // namespace d2server100
