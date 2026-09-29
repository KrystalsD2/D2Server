#pragma once
#include "Compat.h"
#include <array>
#include <cstddef>
#include <string>

namespace d2server100 {

// Parsed Diablo II 1.00 Game.exe startup/config block passed in ECX to
// D2Server QueryInterface slot 0. Game.exe clears exactly 0x376 bytes before
// applying its descriptor table. Only fields directly consumed by retail
// D2Server.dll are exposed here; the remainder intentionally stays opaque.
struct GameStartupConfig100 {
    std::array<u8,0x376> raw{};
};
static_assert(sizeof(GameStartupConfig100)==0x376,"1.00 Game.exe startup block size");

struct GameStartupSignals100 {
    bool present = false;
    // Retail stores this flag but the shipped service core has no later read.
    bool battleNetConfigured = false;
    // This is the real online realm-service gate. Literal ASCII "0" disables
    // Blizzard MCP/CharServ startup even if realms.ini contains endpoints.
    bool mcpEnabled = false;
    bool debugLog = false;
    std::string battleNetIp;
    std::string mcpIp;
};

// Binary-proven offsets consumed by retail D2Server 1.00 initializers.
constexpr std::size_t kGame100_BattleNetIpOffset = 0x046;
constexpr std::size_t kGame100_McpIpOffset       = 0x05E;
constexpr std::size_t kGame100_DebugLogOffset    = 0x1EE;
constexpr std::size_t kGame100_StringFieldBytes  = 0x18;

GameStartupSignals100 CaptureGameStartupSignals100(const GameStartupConfig100* config) noexcept;

// Retail copies Game.exe MCPIP into a working buffer first, then a selected
// realms.ini mcpip= value replaces it when present. This helper models that
// default/override rule only; it does not decide whether online service is on.
std::string ResolveMcpEndpoint100(const GameStartupSignals100& startup,
                                  const std::string& realmMcpIp);

} // namespace d2server100
