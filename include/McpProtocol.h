#pragma once
#include "Compat.h"
#include <array>

namespace d2server100 {

// The MCP dispatcher at VA 0x10005AE0 rejects opcodes >= 0x14.
enum class McpToGameServer : u8 {
    Type00 = 0x00,
    Type01 = 0x01,
    Type02 = 0x02,
    Type03 = 0x03,
    Recreate = 0x04,        // confirmed by embedded diagnostic string
    Type05 = 0x05,
    CreateGame = 0x06,     // confirmed by ToMCP diagnostic string
    JoinGame = 0x07,       // confirmed by ToMCP diagnostic string
    Type0C = 0x0C,
    Type0D = 0x0D,
    Type0F = 0x0F,
    Type12 = 0x12,
    Type13 = 0x13,
};

#pragma pack(push, 1)
struct McpCreateGamePrefix {
    u8 opcode;      // 0x06
    u32 mcpId;      // +0x01
    u32 field05;    // +0x05, forwarded to D2Game #10047
    u8 field09;     // +0x09
    u8 field0A;     // +0x0A
    u8 field0B;     // +0x0B
    char strings[1];// +0x0C: two consecutive NUL-terminated strings
};

struct McpJoinGamePrefix {
    u8 opcode;               // 0x07
    u32 mcpId;               // +0x01
    u32 gameId;              // +0x05
    u32 token;               // +0x09
    u32 characterSaveToken;  // +0x0D
    char names[1];           // +0x11: characterName\0accountName\0
};

struct GameServerCreateReply {
    u8 opcode;       // 0x06
    u32 mcpId;
    u32 serverGameId;
    u32 error;
};
static_assert(sizeof(GameServerCreateReply) == 13);

struct GameServerJoinReply {
    u8 opcode;       // 0x07
    u32 mcpId;
    u32 serverGameId;
    u32 token;
    u32 error;
};
static_assert(sizeof(GameServerJoinReply) == 17);
#pragma pack(pop)

} // namespace d2server100
