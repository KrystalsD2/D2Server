#include "D2Server100.h"
#include "Startup100.h"
#include "ServerRuntime100.h"
#include "Diagnostics100.h"

namespace d2server100 {
const McpHandler100* reconstructedMcpHandlers() noexcept;
RuntimeBindings100& bindings() noexcept { static RuntimeBindings100 v{}; return v; }
PlayerTokenStore& playerTokens() noexcept { static PlayerTokenStore v{}; return v; }
void logLine(const char* text) noexcept { if (bindings().log) bindings().log(text); else DiagnosticLog100(text); }

int D2_FASTCALL ServerStartAdapter(const char* commandLine) {
    return RunServer100(commandLine);
}

const D2ServerInterface100& interfaceTable() noexcept {
    static D2ServerInterface100 table{};
    static bool init=false;
    if (!init) {
        table.start=&ServerStartAdapter;
        const auto* h=reconstructedMcpHandlers();
        for (std::size_t i=0;i<0x14;++i) table.fromMcp[i]=h[i];
        init=true;
    }
    return table;
}
}
D2_EXPORT const d2server100::D2ServerInterface100* D2_CDECL QueryInterface() {
    return &d2server100::interfaceTable();
}
