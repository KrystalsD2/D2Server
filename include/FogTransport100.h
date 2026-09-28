#pragma once
#include "Compat.h"
#include "RealmConfig100.h"
#include <array>

namespace d2server100 {

#pragma pack(push,1)
struct CharServerSlotLayout100 {
    u32 connection;          // +00 (x86 pointer)
    char address[36];        // +04
    u32 reconnectThread;     // +28 (x86 HANDLE)
};
#pragma pack(pop)
static_assert(sizeof(CharServerSlotLayout100)==0x2C,"original CharServ slot is 0x2C bytes");

class FogTransport100 {
public:
    bool start(const RealmConfigEntry100& realm) noexcept;
    void stop() noexcept;
    bool sendMcp(const void* data,std::size_t size) noexcept;
    bool sendCharServer(const void* data,std::size_t size) noexcept;
    void serviceTick() noexcept;
    bool started() const noexcept { return started_; }
    u32 primaryCharServer() const noexcept { return primaryChar_; }

private:
    struct RuntimeSlot { void* connection=nullptr; void* reconnectThread=nullptr; std::array<char,36> address{}; };
    bool openMcp() noexcept;
    bool openChar(u32 index,bool reconnect) noexcept;
    void pollMcp() noexcept;
    void pollChars() noexcept;
    void healthCheck() noexcept;
    void* mcp_=nullptr;
    std::array<char,36> mcpAddress_{};
    std::array<RuntimeSlot,3> chars_{};
    u32 charCount_=0;
    u32 primaryChar_=0;
    u32 lastHealthMs_=0;
    bool started_=false;
};

FogTransport100& fogTransport100() noexcept;
bool RuntimeSendMcp100(const void* data,std::size_t size);
bool RuntimeSendChar100(const void* data,std::size_t size);

} // namespace d2server100
