#include "D2Server100.h"
#include "McpProtocol.h"
#include "Callbacks100.h"
#include "ServerRuntime100.h"
#include <cstring>

namespace d2server100 {
bool SendCreateGameReply(u32 serverGameId, u32 mcpId, u32 error);
bool SendJoinGameReply(u32 mcpId, u32 serverGameId, u32 token, u32 error);

namespace {
int return_success(const u8*, u32) { return 1; }

int D2_FASTCALL Handle00(const u8* p, u32 n) { return return_success(p,n); }
int D2_FASTCALL Handle01(const u8*, u32) {
    // Original handler calls VA 0x10008580. It sends this exact 17-byte
    // server-registration record and returns success.
    std::array<u8,17> packet{};
    packet[0]=0x02;
    write_u32(packet.data()+1,0x49583836u);
    write_u32(packet.data()+5,0x44324456u);
    if (bindings().sendToMcp) bindings().sendToMcp(packet.data(),packet.size());
    return 1;
}
int D2_FASTCALL Handle02(const u8*, u32) {
    // VA 0x100085D0 constructs a 0x109-byte opcode-03 status record with
    // its fixed fields cleared before sending. Unresolved display/archive
    // fields remain zero in this source-equivalent reconstruction.
    std::array<u8,0x109> packet{};
    packet[0]=0x03;
    if (bindings().sendToMcp) bindings().sendToMcp(packet.data(),packet.size());
    return 1;
}
int D2_FASTCALL Handle03(const u8*, u32) {
    // Original handler calls VA 0x10008700, an archive/enumeration status
    // routine. The transport-independent semantics are not reconstructed yet.
    return 1;
}

int D2_FASTCALL HandleRecreate(const u8* packet, u32 length) {
    (void)packet; (void)length;
    // Original VA 0x100057E0 calls Blizzard's assert/fatal path with:
    //   "GTM_RECREATE shouldn't be sent from MCP to Server!"
    //   CORE\\FromMCP.cpp, line 107 (0x6B).
    return 0;
}

int D2_FASTCALL Handle05(const u8* p, u32 n) { return return_success(p,n); }

int D2_FASTCALL HandleCreateGame(const u8* packet, u32 length) {
    // Original function VA 0x10005830 does not explicitly length-check before parsing.
    // Reconstruction adds a minimum guard so malformed test input cannot walk memory.
    if (!packet || length < 13 || packet[0] != 0x06) return 0;

    const u32 mcpId = read_u32(packet + 1);
    const u32 field05 = read_u32(packet + 5);
    const u8 field09 = packet[9];
    const u8 field0A = packet[10];
    const u8 field0B = packet[11];
    const char* textA = reinterpret_cast<const char*>(packet + 12);
    const std::size_t available = length - 12;
    const std::size_t lenA = bounded_strlen(textA, available);
    if (lenA >= available) return 0;
    const std::size_t usedA=lenA+1;
    if (usedA>=available) return 0;
    const char* textB = textA + usedA;
    const std::size_t availableB=available-usedA;
    const std::size_t lenB=bounded_strlen(textB,availableB);
    if (lenB>=availableB) return 0;

    u32 serverGameId = 0;
    auto fn = bindings().d2game.ord10047;
    if (!fn || !bindings().gameCreateEnabled)
        return SendCreateGameReply(0, mcpId, 1) ? 1 : 0;

    const int created = fn(textA, textB, 0, field05, field09, field0A, field0B, &serverGameId);
    if (created) {
        // Binary explicitly masks the returned ID to 16 bits before replying.
        SendCreateGameReply(serverGameId & 0xFFFFu, mcpId, 0);
    } else {
        SendCreateGameReply(0, mcpId, 1);
    }
    return 1;
}

int D2_FASTCALL HandleJoinGame(const u8* packet, u32 length) {
    if (!packet || length < 19 || packet[0] != 0x07) return 0;

    const u32 mcpId = read_u32(packet + 1);
    const u32 gameId = read_u32(packet + 5);
    const u32 token = read_u32(packet + 9);
    const u32 characterSaveToken = read_u32(packet + 13);

    const char* characterName = reinterpret_cast<const char*>(packet + 17);
    const std::size_t avail = length - 17;
    const std::size_t charLen = bounded_strlen(characterName, avail);
    if (charLen >= avail) return 0;
    const std::size_t usedChar=charLen+1;
    if (usedChar>=avail) return 0;
    const char* accountName = characterName + usedChar;
    const std::size_t accountAvail=avail-usedChar;
    const std::size_t accountLen=bounded_strlen(accountName,accountAvail);
    if (accountLen>=accountAvail) return 0;

    playerTokens().upsert(characterName, accountName, characterSaveToken, token, gameId);
    SendJoinGameReply(mcpId, gameId, token, 0);
    return 1;
}

int D2_FASTCALL Handle0C(const u8* packet, u32 length) {
    if (!packet || length < 15 || packet[0] != 0x0C) return 0;
    auto fn = bindings().d2game.ord10048;
    if (!fn) return 0;
    fn(read_u32(packet + 1), packet + 5,
       read_u16(packet + 11), read_u16(packet + 9),
       read_u16(packet + 13), packet + 15);
    return 1;
}

int D2_FASTCALL Handle0D(const u8* packet, u32 length) {
    if (!packet || length < 15 || packet[0] != 0x0D) return 0;
    auto fn = bindings().d2game.ord10049;
    if (!fn) return 0;
    fn(read_u32(packet + 1), packet + 5,
       read_u16(packet + 11), read_u16(packet + 9),
       read_u16(packet + 13), packet + 15);
    return 1;
}

// Original VA 0x100059D0 accepts exactly 0x89 bytes, verifies opcode 0x0F,
// then copies the following 0x88 bytes into 0x100294F8..0x1002957F.
// That exact region is subsequently read by UpdateCharacterLadder.
int D2_FASTCALL Handle0F(const u8* packet, u32 length) {
    if (!packet || length != 0x89 || packet[0] != 0x0F) return 0;
    UpdateLadderStateBlob100(packet + 1, 0x88);
    return 1;
}

int D2_FASTCALL Handle12(const u8* packet, u32 length) {
    if (!packet || length <= 9 || packet[0] != 0x12) return 0;
    // Original formats packet+9 through "Diablo: %s" and forwards the two
    // dwords at +1/+5 to an internal routine. Semantics remain unresolved.
    return 1;
}

int D2_FASTCALL Handle13(const u8* packet, u32 length) {
    if (!packet || length != 9 || packet[0] != 0x13) return 0;
    // Original VA 0x10009D10 logs "Shutdown Remotely by MCP" and sets the
    // global application stop state after beginning resource shutdown.
    RequestServerStop100("Shutdown Remotely by MCP");
    return 1;
}

constexpr McpHandler100 kHandlers[0x14] = {
    &Handle00, &Handle01, &Handle02, &Handle03,
    &HandleRecreate, &Handle05, &HandleCreateGame, &HandleJoinGame,
    nullptr, nullptr, nullptr, nullptr,
    &Handle0C, &Handle0D, nullptr, &Handle0F,
    nullptr, nullptr, &Handle12, &Handle13
};
} // namespace

int D2_FASTCALL DispatchFromMcp(const u8* packet, u32 length) {
    // Faithful to VA 0x10005AE0: nonzero length, opcode < 0x14, null handler => 0.
    if (!packet || length == 0) return 0;
    const u8 opcode = packet[0];
    if (opcode >= 0x14) return 0;
    McpHandler100 fn = kHandlers[opcode];
    return fn ? fn(packet, length) : 0;
}

const McpHandler100* reconstructedMcpHandlers() noexcept { return kHandlers; }

} // namespace d2server100
