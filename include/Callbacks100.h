#pragma once
#include "Compat.h"
#include <cstddef>

namespace d2server100 {

using GenericCallback100 = void (D2_CDECL *)();

struct EventCallbackTable100 {
    GenericCallback100 slot[12];
};
#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(EventCallbackTable100)==0x30, "1.00 D2Game callback table is 12 x 4-byte pointers");
#endif

const EventCallbackTable100& eventCallbackTable100() noexcept;

// Exact 1.00 callback order at D2Server VA 0x10024A90.
void D2_FASTCALL Callback_CloseGame(u32 gameId);
void D2_FASTCALL Callback_LeaveGame(u32 gameId, u32 charClass,
                                    u32 charLevel, u32 expLow, u32 expHigh,
                                    u32 charStatus, const char* characterName,
                                    const char* characterPortrait);
int D2_FASTCALL Callback_GetDatabaseCharacter(const char* characterName, u32 requestTag);
int D2_FASTCALL Callback_SaveDatabaseCharacter(const char* characterName,
                                                const char* accountName,
                                                const void* saveData,
                                                u32 saveSize,
                                                u32 playerData);
void D2_CDECL Callback_ServerLogMessage(u32 count, const char* format, ...);
void D2_FASTCALL Callback_EnterGame(u32 gameId, const char* characterName,
                                   u32 charClass, u32 charLevel, u32 zero);
int D2_FASTCALL Callback_FindPlayerToken(const char* characterName,
                                         u32 token,
                                         u32 gameId,
                                         char* outAccountName,
                                         u32* outCharacterSaveToken);
int D2_FASTCALL Callback_SaveDatabaseGuild(const char* guildTag,
                                            const void* data,
                                            u32 size);
int D2_FASTCALL Callback_UnlockDatabaseCharacter(const char* characterName);
void D2_FASTCALL Callback09(const char* tag, u32 type,
                            const char* text, u32 value);
int D2_FASTCALL Callback_UpdateCharacterLadder(const char* characterName,
                                                u32 charClass,
                                                u32 charLevel,
                                                u32 expLow,
                                                u32 expHigh,
                                                u32 charStatus);
void D2_FASTCALL Callback_UpdateGameInformation(u32 gameId,
                                                const char* characterName,
                                                u32 charClass,
                                                u32 charLevel);

// MCP opcode 0x0F copies exactly 0x88 bytes to the global ladder comparison
// region later read by Callback_UpdateCharacterLadder.
void UpdateLadderStateBlob100(const u8* data, std::size_t size) noexcept;
const u8* LadderStateBlob100() noexcept;

// Original save checksum loop at D2Server VA 0x1000206B.
u32 SaveChecksum100(const void* data, u32 size) noexcept;

} // namespace d2server100
