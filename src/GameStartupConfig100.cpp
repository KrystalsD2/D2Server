#include "GameStartupConfig100.h"

namespace d2server100 {
namespace {
std::string boundedString(const GameStartupConfig100* c,std::size_t offset,std::size_t maxBytes) {
    if (!c || offset>=c->raw.size()) return {};
    const std::size_t available=c->raw.size()-offset;
    const std::size_t limit=maxBytes<available?maxBytes:available;
    const char* p=reinterpret_cast<const char*>(c->raw.data()+offset);
    std::size_t n=0; while(n<limit && p[n]) ++n;
    return std::string(p,n);
}
}

GameStartupSignals100 CaptureGameStartupSignals100(const GameStartupConfig100* config) noexcept {
    GameStartupSignals100 out{};
    if (!config) return out;
    out.present=true;
    out.battleNetIp=boundedString(config,kGame100_BattleNetIpOffset,kGame100_StringFieldBytes);
    out.mcpIp=boundedString(config,kGame100_McpIpOffset,kGame100_StringFieldBytes);
    // These comparisons intentionally match retail exactly. An empty NUL byte
    // is not ASCII '0', so it evaluates true; do not "clean up" this quirk.
    out.battleNetConfigured=config->raw[kGame100_BattleNetIpOffset] != static_cast<u8>('0');
    out.mcpEnabled=config->raw[kGame100_McpIpOffset] != static_cast<u8>('0');
    out.debugLog=config->raw[kGame100_DebugLogOffset] != 0;
    return out;
}

std::string ResolveMcpEndpoint100(const GameStartupSignals100& startup,
                                  const std::string& realmMcpIp) {
    if (!realmMcpIp.empty()) return realmMcpIp;
    return startup.present ? startup.mcpIp : std::string{};
}

} // namespace d2server100
