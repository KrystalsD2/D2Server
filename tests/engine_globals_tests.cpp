#include "EngineGlobals100.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
using namespace d2server100;

static std::vector<std::string> events;
static int D2_FASTCALL LangInit(u32 v){ events.push_back("lang10000:"+std::to_string(v)); return 1; }
static void D2_CDECL LangShutdown(){ events.push_back("lang10001"); }
static void D2_STDCALL CommonInit(u32 a,u32 b,u32 c){
    events.push_back("common10554:"+std::to_string(a)+","+std::to_string(b)+","+std::to_string(c));
}
static void D2_CDECL CommonShutdown(){ events.push_back("common10553"); }
static void D2_CDECL ProfDump(){ events.push_back("common10983"); }

int main(){
    auto& lang=d2langApi100();
    lang.ord10000=&LangInit; lang.ord10001=&LangShutdown;
    auto& common=d2commonApi100();
    common.ord10554=&CommonInit; common.ord10553=&CommonShutdown; common.ord10983=&ProfDump;

    events.clear();
    assert(InitializeEngineGlobals100());
    assert(EngineGlobalsInitialized100());
    assert(events.size()==2);
    assert(events[0]=="lang10000:0");
    assert(events[1]=="common10554:0,1,0");

    // The lifecycle owner is idempotent; do not double-increment D2Lang's
    // retail refcount or reinitialize D2Common tables on repeated requests.
    assert(InitializeEngineGlobals100());
    assert(events.size()==2);

    ShutdownEngineGlobals100();
    assert(!EngineGlobalsInitialized100());
    assert(events.size()==5);
    assert(events[2]=="common10553");
    assert(events[3]=="lang10001");
    assert(events[4]=="common10983");

    ShutdownEngineGlobals100();
    assert(events.size()==5);
    std::cout << "Retail 1.00 D2Lang/D2Common lifecycle tests passed\n";
}
