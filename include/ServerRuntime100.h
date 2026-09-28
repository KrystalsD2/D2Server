#pragma once
#include "Compat.h"
#include <atomic>
namespace d2server100 {
struct D2NetApi100;
class FogTransport100;

// Exact May-26-2000 WinMain D2Net startup sequence:
//   #10003(0,0) -> #10026(MULTICLIENT/default 1) -> #10023(1)
bool InitializeD2Net100(D2NetApi100& net, u32 multiclient) noexcept;

// Retail 1.00 owned teardown order after all game workers have exited:
//   D2Game #10040 -> D2Net #10004 -> MCP/CharServ Fog destruction -> D2Game #10050
//   -> D2Common #10553 -> D2Lang #10001 -> D2Common #10983 profiler dump.
void ShutdownServerRuntime100(D2NetApi100& net, FogTransport100& transport) noexcept;

int RunServer100(const char* commandLine) noexcept;
void RequestServerStop100(const char* reason=nullptr) noexcept;
bool ServerStopRequested100() noexcept;
}
