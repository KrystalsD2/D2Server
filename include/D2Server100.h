#pragma once
#include "Compat.h"
#include "D2GameApi.h"
#include "PlayerToken.h"
#include "GameStartupConfig100.h"
#include <array>

namespace d2server100 {
using McpHandler100 = int (D2_FASTCALL *)(const u8* packet,u32 length);
using ServerStart100 = int (D2_FASTCALL *)(const GameStartupConfig100* startupConfig);
using PacketSendFunction = bool (*)(const void* packet,std::size_t length);
using EventLogFunction = void (*)(const char* text);

struct D2ServerInterface100 {
    ServerStart100 start;
    McpHandler100 fromMcp[0x14];
    void* reserved[4]; // original QueryInterface block continues with four NULL DWORDs
};
#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(D2ServerInterface100)==0x64,"original 1.00 QueryInterface block is 25 DWORDs / 0x64 bytes");
#endif

struct RuntimeBindings100 {
    D2GameApi100 d2game;
    PacketSendFunction sendToMcp = nullptr;
    PacketSendFunction sendToCharServer = nullptr;
    EventLogFunction log = nullptr;
    bool engineRuntimeInitialized = false;
    bool callbacksInstalled = false;
    bool gameDataInstalled = false;
    bool gameCreateEnabled = false;
};

RuntimeBindings100& bindings() noexcept;
PlayerTokenStore& playerTokens() noexcept;
const D2ServerInterface100& interfaceTable() noexcept;
int D2_FASTCALL ServerStartAdapter(const GameStartupConfig100* startupConfig);
int D2_FASTCALL DispatchFromMcp(const u8* packet,u32 length);
void logLine(const char* text) noexcept;
}

D2_EXPORT const d2server100::D2ServerInterface100* D2_CDECL QueryInterface();
