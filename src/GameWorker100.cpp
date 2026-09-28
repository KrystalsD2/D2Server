#include "GameWorker100.h"
#include "D2Server100.h"
#include "D2NetApi100.h"
#include "FogApi100.h"
#include <thread>

namespace d2server100 {
u32 GameWorkerCount100() noexcept {
    auto& f=fogApi100();
    if (f.ord10020) {
        const auto* descriptor=static_cast<const u8*>(f.ord10020());
        if (descriptor) {
            const u32 count=read_u32(descriptor+0x28);
            if (count) return count;
        }
    }
    const unsigned host=std::thread::hardware_concurrency();
    return host ? (u32)host : 1u;
}

GameWorkerStep100 GameWorkerIteration100(void* queue,u32& serial) noexcept {
    GameWorkerStep100 r{};
    auto& g=bindings().d2game; auto& n=d2netApi100();
    if (!queue || !g.ord10043 || !g.ord10045 || !g.ord10003 || !n.ord10022) return r;
    void* due=nullptr;
    r.waitMs=g.ord10043(queue,&due); if (r.waitMs<0) r.waitMs=0;
    r.hadDueTask=due!=nullptr;
    // Original loop pumps when idle, and under continuous due work on the first
    // iteration then each eighth task.
    const bool shouldPump=!due || ((serial++ & 7u)==0u);
    if (shouldPump) {
        r.pumpedNetwork=true;
        if (n.ord10022((u32)r.waitMs)) { g.ord10003(); r.processedNetwork=true; }
    }
    if (due) { g.ord10045(queue,due); r.processedTask=true; }
    return r;
}

void GameWorkerLoop100(std::atomic<bool>& stopFlag) noexcept {
    auto& g=bindings().d2game; if (!g.ord10041) return;
    void* queue=g.ord10041(); if (!queue) return;
    u32 serial=0;
    while (!stopFlag.load(std::memory_order_relaxed)) GameWorkerIteration100(queue,serial);
}
}
