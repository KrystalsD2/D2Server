#include "Startup100.h"
#include "D2Server100.h"
#include "Callbacks100.h"
#include "GameDataHash100.h"

namespace d2server100 {
namespace {
StartupReport100 g_report{};
GameDataHash100 g_gameData{};
u32 g_requiredNonNull = 1;
bool g_shutdown40Done = false;
bool g_shutdown50Done = false;
}

StartupReport100 InitializeEngineRuntime100() noexcept {
    auto& rt=bindings();
    g_report={StartupStage100::NotStarted,false,"starting"};
    g_shutdown40Done=false;
    g_shutdown50Done=false;

    // Portable tests may inject a complete fake table. On Windows, resolve the
    // exact May-2000 ordinals when no complete table is already present.
    if (!rt.d2game.hasBootstrapSet() && !rt.d2game.resolve()) {
        g_report={StartupStage100::Failed,false,"could not resolve exact 1.00 D2Game bootstrap ordinals"};
        return g_report;
    }
    g_report={StartupStage100::D2GameResolved,true,"D2Game ordinals resolved"};

    if (!rt.d2game.ord10046 || !rt.d2game.ord10046()) {
        g_report={StartupStage100::Failed,false,"D2Game #10046 runtime initialization failed"};
        return g_report;
    }
    rt.engineRuntimeInitialized=true;
    rt.callbacksInstalled=false;
    rt.gameDataInstalled=false;
    rt.gameCreateEnabled=false;
    g_report={StartupStage100::RuntimeInitialized,true,"D2Game runtime initialized; transport phase may start"};
    return g_report;
}

StartupReport100 CompleteEngineBootstrap100(bool requestGameDataInstall) noexcept {
    auto& rt=bindings();
    if (!rt.engineRuntimeInitialized || !rt.d2game.ord10023) {
        g_report={StartupStage100::Failed,false,"D2Game runtime phase was not initialized"};
        return g_report;
    }

    // Retail WinMain does not install this table until its MCP connection and
    // Character Server initialization have completed.
    rt.d2game.ord10023((void*)&eventCallbackTable100());
    rt.callbacksInstalled=true;
    g_report={StartupStage100::CallbacksInstalled,true,"12-entry 1.00 callback table installed"};

    if (!requestGameDataInstall) {
        rt.gameDataInstalled=false;
        rt.gameCreateEnabled=false;
        return g_report;
    }
    if (!g_gameData.initializeRuntime() || !g_gameData.readyForD2GameCreate()) {
        g_report={StartupStage100::Failed,false,
                  "could not initialize exact 1.00 SGAMEDATA allocator/vtable (Storm #401/#403)"};
        return g_report;
    }

    rt.d2game.ord10002(g_gameData.layout(),&g_requiredNonNull);
    rt.gameDataInstalled=true;
    g_report={StartupStage100::GameDataInstalled,true,"game-data hash installed"};

    // Original WinMain calls D2Game #10039 immediately after #10002. The
    // exact May-2000 export initializes a D2Game synchronization object.
    rt.d2game.ord10039();
    rt.gameCreateEnabled=true;
    g_report={StartupStage100::PostGameDataInitialized,true,
              "post-game-data initialization complete; CREATEGAME enabled"};
    return g_report;
}

StartupReport100 BootstrapEngine100(bool requestGameDataInstall) noexcept {
    const auto first=InitializeEngineRuntime100();
    if (!first.ok) return first;
    return CompleteEngineBootstrap100(requestGameDataInstall);
}

void BeginEngineShutdown100() noexcept {
    auto& rt=bindings();
    rt.gameCreateEnabled=false;
    if (rt.engineRuntimeInitialized && !g_shutdown40Done && rt.d2game.ord10040) {
        rt.d2game.ord10040();
        g_shutdown40Done=true;
    }
}

void FinalizeEngineShutdown100() noexcept {
    auto& rt=bindings();
    if (rt.engineRuntimeInitialized && !g_shutdown50Done && rt.d2game.ord10050) {
        (void)rt.d2game.ord10050();
        g_shutdown50Done=true;
    }
    rt.gameCreateEnabled=false;
    rt.gameDataInstalled=false;
    rt.callbacksInstalled=false;
    rt.engineRuntimeInitialized=false;
}

void ShutdownEngine100() noexcept {
    BeginEngineShutdown100();
    FinalizeEngineShutdown100();
}

const StartupReport100& lastStartupReport100() noexcept { return g_report; }
}
