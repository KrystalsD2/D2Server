#include "D2Server100.h"
#include "McpProtocol.h"

namespace d2server100 {
bool SendCreateGameReply(u32 serverGameId, u32 mcpId, u32 error);
}

// Original ordinal 10001 resolves through the incremental-link trampoline at
// RVA 0x1325 to VA 0x100087C0, the confirmed GTM_CREATEGAME reply sender.
D2_EXPORT int D2_STDCALL D2Server_Export10001(std::uint32_t serverGameId,
                                              std::uint32_t mcpId,
                                              std::uint32_t error) {
    return d2server100::SendCreateGameReply(serverGameId, mcpId, error) ? 1 : 0;
}

// Original ordinal 10002 resolves RVA 0x10F0 -> VA 0x10009E80, which is a bare RET.
D2_EXPORT void D2_CDECL D2Server_Export10002() {}
