#include "D2NetApi100.h"
#include "ServerRuntime100.h"
#include <cassert>
#include <iostream>
#include <vector>
using namespace d2server100;

struct Call { int ord; u32 a; u32 b; };
static std::vector<Call> calls;
static int D2_STDCALL Init(u32 a,u32 b){ calls.push_back({10003,a,b}); return 0; /* retail caller ignores this */ }
static void D2_CDECL ShutdownControl(){}
static void D2_STDCALL SetCallback(D2NetApi100::HackReportCallback){}
static int D2_STDCALL Wait(u32){ return 0; }
static void D2_STDCALL Mode(u32 v){ calls.push_back({10023,v,0}); }
static void D2_STDCALL Multi(u32 v){ calls.push_back({10026,v,0}); }

static D2NetApi100 makeApi(){
    D2NetApi100 n{};
    n.ord10003=&Init; n.ord10004=&ShutdownControl; n.ord10018=&SetCallback;
    n.ord10022=&Wait; n.ord10023=&Mode; n.ord10026=&Multi;
    return n;
}

int main(){
    auto n=makeApi();
    calls.clear(); assert(InitializeD2Net100(n,7));
    assert(calls.size()==3);
    assert(calls[0].ord==10003 && calls[0].a==0 && calls[0].b==0);
    assert(calls[1].ord==10026 && calls[1].a==7);
    assert(calls[2].ord==10023 && calls[2].a==1);

    calls.clear(); assert(InitializeD2Net100(n,0));
    assert(calls.size()==3 && calls[0].ord==10003);
    assert(calls[1].ord==10026 && calls[1].a==1);
    assert(calls[2].ord==10023 && calls[2].a==1);

    std::cout << "D2Net exact startup sequence tests passed\n";
}
