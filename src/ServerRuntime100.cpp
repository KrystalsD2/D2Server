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
#include <chrono>
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

void ShutdownServerRuntime100(D2NetApi100& net, FogTransport100& transport) noexcept {
    // Exact core ordering at retail D2Server.dll 1.00 VA 0x1000A132..0x1000A159.
    // The original first performs an application/UI list cleanup at 0x100055B0;
    // that list does not exist in this headless reconstruction. Engine/network
    // ordering from the first owned subsystem call onward is preserved here.
    BeginEngineShutdown100();                       // D2Game #10040
    if (net.ord10004) net.ord10004();              // D2Net  #10004
    transport.stop();                              // #10018(NULL), Fog MCP, 3 CharServ slots
    FinalizeEngineShutdown100();                    // D2Game #10050
    ShutdownEngineGlobals100();                     // D2Common #10553, D2Lang #10001, D2Common #10983
    bindings().sendToMcp=nullptr;
    bindings().sendToCharServer=nullptr;
}

int RunServer100(const char* commandLine) noexcept {
    (void)commandLine;
    g_stop.store(false);
    bindings().log=&DiagnosticLog100;
    logLine("D2Server100 v0.4: start");

    // Retail WinMain's first Fog call is #10139 with ECX=1, followed later by
    // ErrorManager #10019 with the literal "D2SDebug" before realm loading.
    // Preserve both; our independent fallback log remains active as well.
    auto& fog=fogApi100();
    if (!fog.resolve() || !fog.transportReady()) { logLine("Fog transport resolve failed"); return 0; }
    if (!fog.ord10139 || !fog.ord10019) { logLine("Fog startup globals missing"); return 0; }
    fog.ord10139(1);
    fog.ord10019("D2SDebug");

    RealmConfig100 config; std::string error;
    const u32 localIp=DetectLocalIPv4HostOrder100();
    if (!config.load("realms.ini",localIp,&error)) { logLine(error.c_str()); return 0; }
    const auto* realm=config.selected(); if (!realm) return 0;

    auto& net=d2netApi100(); if (!net.resolve()) { logLine("D2Net resolve failed"); return 0; }

    // Retail initializes D2Lang and D2Common data tables immediately before
    // the D2Net startup block.
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

    // Retail 1.00 startup is split around its realm transports: #10046 first,
    // then MCP/CharServ, then the callback table / game-data hash / #10039.
    const auto engineFirst=InitializeEngineRuntime100();
    if (!engineFirst.ok) {
        logLine(engineFirst.message);
        ShutdownServerRuntime100(net,fogTransport100());
        return 0;
    }

    bindings().sendToMcp=&RuntimeSendMcp100;
    bindings().sendToCharServer=&RuntimeSendChar100;
    if (!fogTransport100().start(*realm)) {
        logLine("MCP/CharServ Fog transport startup failed");
        ShutdownServerRuntime100(net,fogTransport100());
        return 0;
    }
    logLine("MCP/CharServ transport online");

    const auto engineFinal=CompleteEngineBootstrap100(true);
    if (!engineFinal.ok) {
        logLine(engineFinal.message);
        ShutdownServerRuntime100(net,fogTransport100());
        return 0;
    }

    // Original WinMain calls Fog #10020 and reads descriptor+0x28. Static
    // analysis of Fog proves +0x14 is a SYSTEM_INFO and +0x28 is therefore
    // dwNumberOfProcessors.
    const u32 workerCount=GameWorkerCount100();
    std::vector<std::thread> workers;
    for(u32 i=0;i<workerCount;++i) workers.emplace_back([]{ GameWorkerLoop100(g_stop); });

    while (!g_stop.load()) {
        fogTransport100().serviceTick();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Retail WinMain sets its stop flag and waits for the worker reference
    // count to reach zero before touching D2Game/D2Net/Fog teardown. Joining
    // the reconstructed workers is the source-equivalent ownership barrier.
    for(auto& t:workers) if(t.joinable()) t.join();
    ShutdownServerRuntime100(net,fogTransport100());
    logLine("D2Server100 v0.4: stopped");
    return 1;
}
}
