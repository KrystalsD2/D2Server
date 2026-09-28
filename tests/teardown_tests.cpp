#include "D2Server100.h"
#include "D2NetApi100.h"
#include "EngineGlobals100.h"
#include "FogApi100.h"
#include "FogTransport100.h"
#include "RealmConfig100.h"
#include "ServerRuntime100.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
using namespace d2server100;

static std::vector<std::string> events;
static int nextHandle=1;

static void* D2_STDCALL FogCreate(const char*,u32,const char*) {
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(nextHandle++));
}
static void D2_STDCALL FogDestroy(void* h) {
    events.push_back("fog48:"+std::to_string((std::uintptr_t)h));
}
static int D2_STDCALL FogSend(void*,const void*,u32){ return 1; }
static int D2_STDCALL FogControl(void*,const void*,u32){ return 1; }
static int D2_STDCALL FogRecv(void*,void*,u32){ return 0; }
static void D2_STDCALL FogStart(void*){}
static void* D2_STDCALL FogStartAsync(void*){ return reinterpret_cast<void*>(9); }
static void D2_STDCALL FogConfigure(void*,FogApi100::ClientPacketCallback,void*){}
static void* D2_STDCALL FogActive(void* h){ return h; }

static int D2_STDCALL NetInit(u32,u32){ return 1; }
static void D2_CDECL NetShutdown(){ events.push_back("net10004"); }
static void D2_STDCALL NetHack(D2NetApi100::HackReportCallback cb){
    if (!cb) events.push_back("net10018:null");
}
static int D2_STDCALL NetWait(u32){ return 0; }
static void D2_STDCALL NetMode(u32){}
static void D2_STDCALL NetMulti(u32){}

static void D2_CDECL Game40(){ events.push_back("game10040"); }
static int D2_CDECL Game50(){ events.push_back("game10050"); return 1; }
static int D2_FASTCALL LangInit(u32){ return 1; }
static void D2_CDECL LangShutdown(){ events.push_back("lang10001"); }
static void D2_STDCALL CommonInit(u32,u32,u32){}
static void D2_CDECL CommonShutdown(){ events.push_back("common10553"); }
static void D2_CDECL ProfDump(){ events.push_back("common10983"); }

int main(){
    auto& f=fogApi100();
    f.ord10047=&FogCreate; f.ord10048=&FogDestroy; f.ord10049=&FogSend;
    f.ord10050=&FogControl; f.ord10051=&FogRecv; f.ord10052=&FogStart;
    f.ord10053=&FogStartAsync; f.ord10056=&FogConfigure; f.ord10058=&FogActive;

    auto& n=d2netApi100();
    n.ord10003=&NetInit; n.ord10004=&NetShutdown; n.ord10018=&NetHack;
    n.ord10022=&NetWait; n.ord10023=&NetMode; n.ord10026=&NetMulti;

    RealmConfigEntry100 realm;
    realm.mcpIp="127.0.0.1";
    realm.charServerCount=2;
    realm.charServers[0]="127.0.0.2";
    realm.charServers[1]="127.0.0.3";
    assert(fogTransport100().start(realm));

    auto& rt=bindings();
    rt.d2game.ord10040=&Game40;
    rt.d2game.ord10050=&Game50;
    rt.engineRuntimeInitialized=true;
    rt.callbacksInstalled=true;
    rt.gameDataInstalled=true;
    rt.gameCreateEnabled=true;
    rt.sendToMcp=&RuntimeSendMcp100;
    rt.sendToCharServer=&RuntimeSendChar100;

    auto& lang=d2langApi100();
    lang.ord10000=&LangInit; lang.ord10001=&LangShutdown;
    auto& common=d2commonApi100();
    common.ord10554=&CommonInit; common.ord10553=&CommonShutdown; common.ord10983=&ProfDump;
    assert(InitializeEngineGlobals100());

    events.clear();
    ShutdownServerRuntime100(n,fogTransport100());

    // Exact subsystem order from retail WinMain 0x1000A132..0x1000A159.
    assert(events.size()==10);
    assert(events[0]=="game10040");
    assert(events[1]=="net10004");
    assert(events[2]=="net10018:null");
    assert(events[3]=="fog48:1");   // MCP
    assert(events[4]=="fog48:2");   // CharServ #0
    assert(events[5]=="fog48:3");   // CharServ #1
    assert(events[6]=="game10050");
    assert(events[7]=="common10553");
    assert(events[8]=="lang10001");
    assert(events[9]=="common10983");

    assert(!rt.engineRuntimeInitialized && !rt.callbacksInstalled &&
           !rt.gameDataInstalled && !rt.gameCreateEnabled);
    assert(rt.sendToMcp==nullptr && rt.sendToCharServer==nullptr);

    // Split engine phases are idempotent after the completed teardown.
    ShutdownServerRuntime100(n,fogTransport100());
    assert(events.size()==12); // net10004 + net10018:null only; no duplicate engine/global calls
    assert(events[10]=="net10004" && events[11]=="net10018:null");

    std::cout << "Retail 1.00 core teardown-order tests passed\n";
}
