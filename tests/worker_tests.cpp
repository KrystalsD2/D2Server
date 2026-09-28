#include "GameWorker100.h"
#include "D2Server100.h"
#include "D2NetApi100.h"
#include "FogApi100.h"
#include <array>
#include <cassert>
#include <iostream>
using namespace d2server100;
static void* due=(void*)0x1234; static int pumps=0,nets=0,tasks=0;
static s32 D2_FASTCALL GetDue(void*,void** out){*out=due; return -5;}
static void D2_FASTCALL DoTask(void*,void*){++tasks;}
static int NetMsgs(){++nets;return 1;}
static int D2_STDCALL Pump(u32 ms){assert(ms==0);++pumps;return 1;}
static std::array<u8,0x40> descriptor{};
static const void* D2_CDECL GetDescriptor(){ return descriptor.data(); }
int main(){
    bindings().d2game.ord10043=&GetDue; bindings().d2game.ord10045=&DoTask; bindings().d2game.ord10003=&NetMsgs;
    d2netApi100().ord10022=&Pump;
    u32 serial=0;
    auto a=GameWorkerIteration100((void*)1,serial); assert(a.waitMs==0&&a.pumpedNetwork&&a.processedTask&&pumps==1&&nets==1&&tasks==1);
    for(int i=0;i<7;++i) GameWorkerIteration100((void*)1,serial);
    assert(pumps==1 && tasks==8);
    GameWorkerIteration100((void*)1,serial); assert(pumps==2 && tasks==9); // every eighth after first
    due=nullptr; GameWorkerIteration100((void*)1,serial); assert(pumps==3);

    // Fog #10020 descriptor +0x28 is SYSTEM_INFO::dwNumberOfProcessors.
    write_u32(descriptor.data()+0x28,6);
    fogApi100().ord10020=&GetDescriptor;
    assert(GameWorkerCount100()==6);

    std::cout << "game worker cadence/count tests passed\n";
}
