#include "GameStartupConfig100.h"
#include <cassert>
#include <cstring>
#include <string>
using namespace d2server100;

int main() {
    GameStartupConfig100 c{};
    static_assert(sizeof(c)==0x376,"layout");
    std::memcpy(c.raw.data()+kGame100_BattleNetIpOffset,"10.0.0.9",9);
    std::memcpy(c.raw.data()+kGame100_McpIpOffset,"10.0.0.10",10);
    c.raw[kGame100_DebugLogOffset]=1;
    auto s=CaptureGameStartupSignals100(&c);
    assert(s.present && s.mcpEnabled && s.battleNetConfigured && s.debugLog);
    assert(s.battleNetIp=="10.0.0.9" && s.mcpIp=="10.0.0.10");
    assert(ResolveMcpEndpoint100(s,"")=="10.0.0.10");
    assert(ResolveMcpEndpoint100(s,"192.168.1.5")=="192.168.1.5");

    c.raw[kGame100_McpIpOffset]='0'; c.raw[kGame100_McpIpOffset+1]=0;
    s=CaptureGameStartupSignals100(&c);
    assert(!s.mcpEnabled);
    // A realm override does not re-enable the path; caller gates on mcpEnabled.
    assert(ResolveMcpEndpoint100(s,"192.168.1.5")=="192.168.1.5");

    c.raw[kGame100_BattleNetIpOffset]='0'; c.raw[kGame100_BattleNetIpOffset+1]=0;
    s=CaptureGameStartupSignals100(&c); assert(!s.battleNetConfigured);
    assert(!CaptureGameStartupSignals100(nullptr).present);
    return 0;
}
