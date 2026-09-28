#include "FogTransport100.h"
#include "FogApi100.h"
#include "D2NetApi100.h"
#include "D2Server100.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using namespace d2server100;

struct Created { std::string address; u32 port; std::string identity; void* handle; };
static int nextId=1;
static std::vector<Created> created;
static std::vector<std::vector<u8>> sends;
static std::vector<std::vector<u8>> controls;
static std::vector<FogApi100::ClientPacketCallback> configuredCallbacks;
static std::vector<D2NetApi100::HackReportCallback> hackCallbacks;

static void* D2_STDCALL Create(const char* a,u32 p,const char* id){
    void* h=(void*)(uintptr_t)(nextId++);
    created.push_back({a?a:"",p,id?id:"",h}); return h;
}
static void D2_STDCALL Destroy(void*){}
static int D2_STDCALL Send(void*,const void* p,u32 n){sends.emplace_back((const u8*)p,(const u8*)p+n);return 1;}
static int D2_STDCALL Control(void*,const void* p,u32 n){controls.emplace_back((const u8*)p,(const u8*)p+n);return 1;}
static int D2_STDCALL Recv(void*,void*,u32){return 0;}
static void D2_STDCALL Start(void*){}
static void* D2_STDCALL StartAsync(void*){return (void*)9;}
static void D2_STDCALL Configure(void*,FogApi100::ClientPacketCallback cb,void*){configuredCallbacks.push_back(cb);}
static void* D2_STDCALL Active(void* p){return p;}

static int D2_STDCALL NetInit(u32,u32){return 1;}
static void D2_CDECL NetShutdown(){}
static void D2_STDCALL SetHack(D2NetApi100::HackReportCallback cb){hackCallbacks.push_back(cb);}
static int D2_STDCALL NetWait(u32){return 0;}
static void D2_STDCALL NetMode(u32){}
static void D2_STDCALL NetMulti(u32){}

int main(){
    auto& f=fogApi100();
    f.ord10047=&Create;f.ord10048=&Destroy;f.ord10049=&Send;f.ord10050=&Control;
    f.ord10051=&Recv;f.ord10052=&Start;f.ord10053=&StartAsync;f.ord10056=&Configure;f.ord10058=&Active;
    auto& n=d2netApi100();
    n.ord10003=&NetInit;n.ord10004=&NetShutdown;n.ord10018=&SetHack;n.ord10022=&NetWait;n.ord10023=&NetMode;n.ord10026=&NetMulti;

    RealmConfigEntry100 r; r.mcpIp="127.0.0.1";r.charServerCount=2;r.charServers[0]="127.0.0.2";r.charServers[1]="127.0.0.3";
    assert(fogTransport100().start(r));
    assert(created.size()==3);
    assert(created[0].address=="127.0.0.1" && created[0].port==6112 && created[0].identity=="GameServer->MCP");
    assert(created[1].port==6113 && created[1].identity=="Game Server->Char Server #0");
    assert(created[2].port==6113 && created[2].identity=="Game Server->Char Server #1");

    // MCP uses the non-null fastcall pre-dispatch callback; CharServ uses null.
    assert(configuredCallbacks.size()==3 && configuredCallbacks[0]!=nullptr && configuredCallbacks[1]==nullptr && configuredCallbacks[2]==nullptr);

    // Exact control hellos: MCP 02 then CharServ 05 / 05.
    assert(controls.size()==3);
    assert(controls[0]==std::vector<u8>({2}));
    assert(controls[1]==std::vector<u8>({5}));
    assert(controls[2]==std::vector<u8>({5}));

    // Immediately after MCP hello the original sends a framed one-byte opcode 01.
    assert(!sends.empty() && sends[0]==std::vector<u8>({1}));
    // start() begins with stop(), so a null callback may be recorded first; the
    // final callback after MCP open must be the installed ECX/fastcall bridge.
    assert(!hackCallbacks.empty() && hackCallbacks.back()!=nullptr);

    const u8 m[]={6,1,2}; assert(RuntimeSendMcp100(m,3)); assert(sends.back()==std::vector<u8>(m,m+3));
    const u8 c[]={3,7}; assert(RuntimeSendChar100(c,2)); assert(sends.back()==std::vector<u8>(c,c+2));

    // Exercise the exact D2Net #10018 register-ABI bridge: it emits MCP 0x09.
    auto hack=hackCallbacks.back();
    hack(0x01020304u);
    assert(sends.back().size()==5 && sends.back()[0]==0x09 && read_u32(sends.back().data()+1)==0x01020304u);

    fogTransport100().serviceTick();
    fogTransport100().stop();
    assert(!hackCallbacks.empty() && hackCallbacks.back()==nullptr);
    std::cout << "Fog MCP/CharServ binary-parity transport tests passed\n";
}
