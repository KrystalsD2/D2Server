#include "Startup100.h"
#include "D2Server100.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
using namespace d2server100;

static std::vector<std::string> calls;
static int D2_CDECL F46(){ calls.push_back("game10046"); return 1; }
static void D2_STDCALL F23(void*){ calls.push_back("game10023"); }
static void D2_STDCALL F02(void*,void*){ calls.push_back("game10002"); }
static void D2_CDECL F39(){ calls.push_back("game10039"); }
static void D2_CDECL F40(){}
static int D2_CDECL F50(){return 1;}
static int D2_CDECL F03(){return 0;}
static void* D2_CDECL F41(){return nullptr;}
static s32 D2_FASTCALL F43(void*,void**){return -1;}
static void D2_FASTCALL F45(void*,void*){}
static int D2_STDCALL F07(u32,const void*,u16,u16,u32){return 0;}
static int D2_STDCALL F47(const char*,const char*,u32,u32,u8,u8,u8,u32*){return 0;}
static void D2_STDCALL F48(u32,const u8*,u16,u16,u16,const u8*){}

static void installFakeD2Game(){
    auto& a=bindings().d2game;
    a.ord10046=&F46; a.ord10023=&F23; a.ord10002=&F02; a.ord10039=&F39;
    a.ord10040=&F40; a.ord10050=&F50; a.ord10003=&F03; a.ord10041=&F41;
    a.ord10043=&F43; a.ord10045=&F45; a.ord10007=&F07; a.ord10047=&F47;
    a.ord10048=&F48; a.ord10049=&F48;
}

int main(){
    installFakeD2Game();
    auto first=InitializeEngineRuntime100();
    assert(first.ok && first.stage==StartupStage100::RuntimeInitialized);
    assert(calls.size()==1 && calls[0]=="game10046");
    assert(bindings().engineRuntimeInitialized);
    assert(!bindings().callbacksInstalled);

    // This marker represents the retail MCP + CharServ startup block. The
    // second engine phase must occur strictly after it.
    calls.push_back("transport");
    auto second=CompleteEngineBootstrap100(false);
    assert(second.ok && second.stage==StartupStage100::CallbacksInstalled);
    assert(calls.size()==3);
    assert(calls[0]=="game10046");
    assert(calls[1]=="transport");
    assert(calls[2]=="game10023");
    assert(bindings().callbacksInstalled);
    assert(!bindings().gameDataInstalled);
    assert(!bindings().gameCreateEnabled);

    std::cout << "split retail engine/transport startup-order test passed\n";
}
