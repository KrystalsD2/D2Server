#include "D2Server100.h"
#include "McpProtocol.h"

namespace d2server100 {

bool SendCreateGameReply(u32 serverGameId, u32 mcpId, u32 error) {
    GameServerCreateReply reply{};
    reply.opcode = 0x06;
    reply.mcpId = mcpId;
    reply.serverGameId = serverGameId;
    reply.error = error;
    return bindings().sendToMcp ? bindings().sendToMcp(&reply, sizeof(reply)) : false;
}

bool SendJoinGameReply(u32 mcpId, u32 serverGameId, u32 token, u32 error) {
    GameServerJoinReply reply{};
    reply.opcode = 0x07;
    reply.mcpId = mcpId;
    reply.serverGameId = serverGameId;
    reply.token = token;
    reply.error = error;
    return bindings().sendToMcp ? bindings().sendToMcp(&reply, sizeof(reply)) : false;
}

} // namespace d2server100
