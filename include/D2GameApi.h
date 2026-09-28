#pragma once
#include "Compat.h"

namespace d2server100 {

struct D2GameApi100 {
    // Exact 1.00 exports verified against the May 26 2000 D2Game.dll.
    using Ord10046_RuntimeInit = int (D2_CDECL *)();
    using Ord10023_SetServerCallbacks = void (D2_STDCALL *)(void* callbackTable);
    using Ord10002_SetGameData = void (D2_STDCALL *)(void* gameDataHash, void* requiredNonNull);
    using Ord10039_PostGameDataInit = void (D2_CDECL *)();
    using Ord10040_ShutdownStage = void (D2_CDECL *)();
    using Ord10050_RuntimeShutdown = int (D2_CDECL *)();

    // Original D2Server game worker / task scheduler exports.
    using Ord10003_ProcessNetworkMessages = int (D2_CDECL *)();
    using Ord10041_CreateTaskQueue = void* (D2_CDECL *)();
    using Ord10043_GetDueTask = s32 (D2_FASTCALL *)(void* taskQueue, void** outDueTask);
    using Ord10045_ProcessGameTask = void (D2_FASTCALL *)(void* taskQueue, void* dueTask);

    // D2Game #10007 ends with RET 0x14: exactly five stack arguments.
    using Ord10007_DeliverDatabaseCharacter = int (D2_STDCALL *)(
        u32 clientOrGameKey,
        const void* saveData,
        u16 chunkSize,
        u16 totalSize,
        u32 modeOrLock);

    // #10047 is the CREATEGAME engine entry used by MCP opcode 0x06.
    using Ord10047_CreateGame = int (D2_STDCALL *)(
        const char* gameName,
        const char* gamePassword,
        u32 zero,
        u32 gameFlags,
        u8 templateId,
        u8 reservedA,
        u8 reservedB,
        u32* outServerGameId);

    using Ord10048 = void (D2_STDCALL *)(u32 field01, const u8* field05,
                                        u16 field0B, u16 field09,
                                        u16 field0D, const u8* field0F);
    using Ord10049 = Ord10048;

    void* module = nullptr;
    Ord10046_RuntimeInit ord10046 = nullptr;
    Ord10023_SetServerCallbacks ord10023 = nullptr;
    Ord10002_SetGameData ord10002 = nullptr;
    Ord10039_PostGameDataInit ord10039 = nullptr;
    Ord10040_ShutdownStage ord10040 = nullptr;
    Ord10050_RuntimeShutdown ord10050 = nullptr;
    Ord10003_ProcessNetworkMessages ord10003 = nullptr;
    Ord10041_CreateTaskQueue ord10041 = nullptr;
    Ord10043_GetDueTask ord10043 = nullptr;
    Ord10045_ProcessGameTask ord10045 = nullptr;
    Ord10007_DeliverDatabaseCharacter ord10007 = nullptr;
    Ord10047_CreateGame ord10047 = nullptr;
    Ord10048 ord10048 = nullptr;
    Ord10049 ord10049 = nullptr;

    bool resolve(const char* moduleName = "D2Game.dll") noexcept;
    void release() noexcept;
    bool hasBootstrapSet() const noexcept;
};

} // namespace d2server100
