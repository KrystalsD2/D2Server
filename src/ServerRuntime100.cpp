#include "ServerRuntime100.h"
#include "D2Server100.h"
#include "D2NetApi100.h"
#include "EngineGlobals100.h"
#include "FogApi100.h"
#include "FogTransport100.h"
#include "RealmConfig100.h"
#include "Startup100.h"
#include "GameWorker100.h"
#include "Diagnostics100.h"
#include "GameStartupConfig100.h"
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

namespace d2server100 {
namespace { std::atomic<bool> g_stop{false}; }
void RequestServerStop100(const char* reason) noexcept { if(reason) logLine(reason); g_stop.store(true); }
bool ServerStopRequested100() noexcept { return g_stop.load(); }

bool InitializeD2Net100(D2NetApi100& net, u32 multiclient) noexcept {
    if (!net.ready()) return false;

    // Exact retail call sequence at D2Server.dll 1.00 VA 0x10009F58.
    // Import-thunk/IAT mapping proves 0x1000BFE2 = #10003,
    // 0x1000BFDC = #10026 and 0x1000BFD6 = #10023.  The caller does
    // not test #10003's return value.
    (void)net.ord10003(0,0);
    net.ord10026(multiclient ? multiclient : 1);
    net.ord10023(1);
    return true;
}

void ShutdownServerRuntime100(D2NetApi100& net, FogTransport100& transport, bool onlineTransportStarted) noexcept {
    // Exact core ordering at retail D2Server.dll 1.00 VA 0x1000A132..0x1000A159.
    // The original first performs an application/UI list cleanup at 0x100055B0;
    // that list does not exist in this headless reconstruction. Engine/network
    // ordering from the first owned subsystem call onward is preserved here.
    BeginEngineShutdown100();                       // D2Game #10040
    if (net.ord10004) net.ord10004();              // D2Net  #10004
    if (onlineTransportStarted) transport.stop(); // retail gates MCP/CharServ teardown on the same online-service flag
    FinalizeEngineShutdown100();                    // D2Game #10050
    ShutdownEngineGlobals100();                     // D2Common #10553, D2Lang #10001, D2Common #10983
    bindings().sendToMcp=nullptr;
    bindings().sendToCharServer=nullptr;
}

int RunServer100(const GameStartupConfig100* startupConfig) noexcept {
    g_stop.store(false);
    const GameStartupSignals100 startup=CaptureGameStartupSignals100(startupConfig);
    // Non-Game.exe harness callers historically passed nullptr and expected the
    // online service path. Preserve that test/compatibility behavior only for
    // null startup objects; real Game.exe callers obey MCPIP == "0" exactly.
    const bool onlineServices = startup.present ? startup.mcpEnabled : true;
    bool onlineTransportStarted=false;

    bindings().log=&DiagnosticLog100;
    logLine("D2Server100 v0.4.1: start");
    if (startup.present) {
        char line[512]{};
        std::snprintf(line,sizeof(line),
                      "Game.exe startup block: size=0x376 onlineServices=%u debugLog=%u bnetFlag=%u mcpIp='%s' bnetIp='%s'",
                      onlineServices?1u:0u,startup.debugLog?1u:0u,startup.battleNetConfigured?1u:0u,
                      startup.mcpIp.c_str(),startup.battleNetIp.c_str());
        logLine(line);
        if (startup.debugLog && !SetD2CommonDebugLog100(true))
            logLine("Game.exe DEBUG.LOG requested but D2Common #10980 was unavailable");
    } else {
        logLine("Game.exe startup block: null compatibility caller; online service path retained");
    }

    // Retail WinMain's first Fog call is #10139 with ECX=1, followed later by
    // ErrorManager #10019 with the literal "D2SDebug" before realm loading.
    auto& fog=fogApi100();
    if (!fog.resolve() || !fog.transportReady()) { logLine("Fog transport resolve failed"); return 0; }
    if (!fog.ord10139 || !fog.ord10019) { logLine("Fog startup globals missing"); return 0; }
    fog.ord10139(1);
    fog.ord10019("D2SDebug");

    // Realm selection happens before retail checks the online-service flag.
    // ADMINPASS/PUBLICPASS are parsed/stored by the retail config layer but no
    // service-core read or MCP registration use has been found in this binary.
    RealmConfig100 config; std::string error;
    const u32 localIp=DetectLocalIPv4HostOrder100();
    if (!config.load("realms.ini",localIp,&error)) { logLine(error.c_str()); return 0; }
    const auto* realm=config.selected(); if (!realm) return 0;

    // Host MCPIP is the initial/default working address. A selected realm's
    // mcpip= replaces it when present. MCPIP == literal "0" still disables the
    // entire Blizzard MCP(:6112)+CharServ(:6113) block regardless of override.
    RealmConfigEntry100 effectiveRealm=*realm;
    if (onlineServices) effectiveRealm.mcpIp=ResolveMcpEndpoint100(startup,realm->mcpIp);

    auto& net=d2netApi100(); if (!net.resolve()) { logLine("D2Net resolve failed"); return 0; }
    if (!InitializeEngineGlobals100()) {
        logLine("D2Lang/D2Common global initialization failed");
        ShutdownEngineGlobals100();
        return 0;
    }

    if (!InitializeD2Net100(net,realm->multiclient)) {
        logLine("D2Net initialization failed");
        ShutdownEngineGlobals100();
        return 0;
    }
    logLine("D2Net initialized on original TCP 4000 path");

    const auto engineFirst=InitializeEngineRuntime100();
    if (!engineFirst.ok) {
        logLine(engineFirst.message);
        ShutdownServerRuntime100(net,fogTransport100(),false);
        return 0;
    }

    if (onlineServices) {
        bindings().sendToMcp=&RuntimeSendMcp100;
        bindings().sendToCharServer=&RuntimeSendChar100;
        if (!fogTransport100().start(effectiveRealm)) {
            logLine("Blizzard realm-control MCP/CharServ transport startup failed");
            ShutdownServerRuntime100(net,fogTransport100(),false);
            bindings().sendToMcp=nullptr;
            bindings().sendToCharServer=nullptr;
            return 0;
        }
        onlineTransportStarted=true;
        logLine("Blizzard realm-control online: MCP :6112 + CharServ :6113");
    } else {
        bindings().sendToMcp=nullptr;
        bindings().sendToCharServer=nullptr;
        logLine("Game.exe MCPIP='0': Blizzard MCP/CharServ online-service block skipped");
    }

    // Retail rejoins the common engine path here whether online service was
    // enabled or not: install callbacks/game-data, #10039, then workers.
    const auto engineFinal=CompleteEngineBootstrap100(true);
    if (!engineFinal.ok) {
        logLine(engineFinal.message);
        ShutdownServerRuntime100(net,fogTransport100(),onlineTransportStarted);
        return 0;
    }

    const u32 workerCount=GameWorkerCount100();
    std::vector<std::thread> workers;
    for(u32 i=0;i<workerCount;++i) workers.emplace_back([]{ GameWorkerLoop100(g_stop); });

    while (!g_stop.load()) {
        if (onlineTransportStarted) fogTransport100().serviceTick();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    for(auto& t:workers) if(t.joinable()) t.join();
    ShutdownServerRuntime100(net,fogTransport100(),onlineTransportStarted);
    logLine("D2Server100 v0.4.1: stopped");
    return 1;
}
}
