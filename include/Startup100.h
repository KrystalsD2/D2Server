#pragma once
#include "Compat.h"

namespace d2server100 {
enum class StartupStage100 : u32 {
    NotStarted=0,
    D2GameResolved,
    RuntimeInitialized,
    CallbacksInstalled,
    GameDataInstalled,
    PostGameDataInitialized,
    Failed
};
struct StartupReport100 {
    StartupStage100 stage=StartupStage100::NotStarted;
    bool ok=false;
    const char* message="not started";
};
// Retail 1.00 startup is deliberately split around the realm transports:
//   D2Game #10046
//   MCP + CharServ Fog transports / D2Net #10018
//   D2Game #10023 callbacks -> #10002 game data -> #10039
// The integrated server uses the two phases below. BootstrapEngine100 remains a
// convenience for isolated callers that do not own the Fog transport layer.
StartupReport100 InitializeEngineRuntime100() noexcept;
StartupReport100 CompleteEngineBootstrap100(bool requestGameDataInstall=false) noexcept;
StartupReport100 BootstrapEngine100(bool requestGameDataInstall=false) noexcept;

// Exact core split observed in retail 1.00 WinMain teardown:
//   D2Game #10040 ... external D2Net/Fog teardown ... D2Game #10050.
// Keeping these stages separate prevents the transport layer from being destroyed
// too early while D2Game #10040 is still retiring games/callback-owned state.
void BeginEngineShutdown100() noexcept;
void FinalizeEngineShutdown100() noexcept;

// Convenience for isolated callers that have no D2Net/Fog runtime between the two
// engine phases. The integrated server uses the split calls above.
void ShutdownEngine100() noexcept;
const StartupReport100& lastStartupReport100() noexcept;
}
