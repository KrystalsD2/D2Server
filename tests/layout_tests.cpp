#include "GameDataHash100.h"
#include "Callbacks100.h"
#include "D2Server100.h"
#include "FogTransport100.h"
#include <cassert>
#include <iostream>
using namespace d2server100;
int main(){
    static_assert(sizeof(GameDataHashLayout100)==0x68);
    static_assert(sizeof(GameDataBucket100)==0x0C);
    static_assert(sizeof(CharServerSlotLayout100)==0x2C);
    GameDataHash100 h;
    assert(h.layout()->capacity==4);
    assert(h.layout()->bucketCount==4);
    assert(h.layout()->hashMask==3);
    assert(!h.readyForD2GameCreate());
    const auto& qi=interfaceTable(); for(auto p:qi.reserved) assert(p==nullptr);
    const auto& cb=eventCallbackTable100();
    for (int i=0;i<12;++i) assert(cb.slot[i]!=nullptr);
    std::cout << "1.00 layout/callback table tests passed\n";
}
