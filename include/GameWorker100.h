#pragma once
#include "Compat.h"
#include <atomic>

namespace d2server100 {
struct GameWorkerStep100 {
    s32 waitMs=0;
    bool hadDueTask=false;
    bool pumpedNetwork=false;
    bool processedNetwork=false;
    bool processedTask=false;
};

// Returns the original worker-count source: Fog #10020 descriptor +0x28,
// which is SYSTEM_INFO::dwNumberOfProcessors. Falls back to the host CPU count
// only when Fog's descriptor is unavailable in a partial reconstruction boot.
u32 GameWorkerCount100() noexcept;

// Executes one reconstructed iteration. Exposed for deterministic tests.
GameWorkerStep100 GameWorkerIteration100(void* taskQueue,u32& taskSerial) noexcept;
void GameWorkerLoop100(std::atomic<bool>& stopFlag) noexcept;
}
