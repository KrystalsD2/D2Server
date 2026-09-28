#include "FogTransport100.h"
#include "FogApi100.h"
#include "D2NetApi100.h"
#include "D2Server100.h"
#include "CharServBridge100.h"
#include "ServerRuntime100.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace d2server100 {
namespace {
u32 tickMs() noexcept {
    using namespace std::chrono;
    return (u32)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
void copyAddress(std::array<char,36>& out,const std::string& in) {
    out.fill(0); const auto n=std::min<std::size_t>(35,in.size()); std::memcpy(out.data(),in.data(),n);
}

// Original Fog callback VA 0x10005B30 performs a bounded opcode lookup through
// a second 20-entry table at 0x10026CE0. That table is zero-filled in the stock
// 1.00 image and has no in-module initializer, so stock pre-dispatch returns 0.
int D2_FASTCALL FogMcpPreDispatch100(const u8* packet,u32 length,void* context) {
    (void)context;
    if (!packet || !length || packet[0]>=0x14) return 0;
    return 0;
}

// D2Net #10018 -> Fog #10126 stores this callback and Fog later invokes it
// with the 32-bit value in ECX. The original D2Server callback sends MCP 0x09.
void D2_FASTCALL HackReportToMcp100(u32 ipv4OrKey) {
    u8 packet[5]{};
    packet[0]=0x09;
    write_u32(packet+1,ipv4OrKey);
    fogTransport100().sendMcp(packet,sizeof(packet));
}

bool slotUsableAddress(const std::array<char,36>& a) noexcept {
    return a[0] != '\0' && a[0] != '0';
}
}

FogTransport100& fogTransport100() noexcept { static FogTransport100 t{}; return t; }
bool RuntimeSendMcp100(const void* p,std::size_t n){ return fogTransport100().sendMcp(p,n); }
bool RuntimeSendChar100(const void* p,std::size_t n){ return fogTransport100().sendCharServer(p,n); }

bool FogTransport100::start(const RealmConfigEntry100& realm) noexcept {
    // A restart must tear down previous ownership, but retail startup does not
    // issue a synthetic D2Net #10018(NULL) before its first MCP connection.
    if (started_ || mcp_ || charCount_) stop();
    auto& fog=fogApi100();
    if (!(fog.transportReady() || fog.resolve()) || !fog.transportReady()) return false;
    copyAddress(mcpAddress_,realm.mcpIp);
    charCount_=std::min<u32>(3,realm.charServerCount);
    for (u32 i=0;i<charCount_;++i) copyAddress(chars_[i].address,realm.charServers[i]);
    if (!openMcp()) return false;

    // Original VA 0x10002640 creates/configures all three possible CharServ
    // slots, sends hello 0x05, then waits in 100 ms scans until at least one
    // Fog transport reports active.
    bool createdAny=false;
    for (u32 i=0;i<charCount_;++i)
        if (slotUsableAddress(chars_[i].address) && openChar(i,false)) createdAny=true;
    if (!createdAny) { stop(); return false; }

    bool activeAny=false;
    while (!activeAny && !ServerStopRequested100()) {
        for (u32 i=0;i<charCount_;++i) {
            if (chars_[i].connection && fog.ord10058(chars_[i].connection)) {
                activeAny=true;
                break;
            }
        }
        if (!activeAny) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (!activeAny) { stop(); return false; }

    // Stock code seeds a pseudo-random starting slot then walks to a connected
    // one. We use the current tick as the equivalent non-semantic seed.
    const u32 seed=charCount_ ? (tickMs()%charCount_) : 0;
    primaryChar_=charCount_ ? ((seed+1)%charCount_) : 0;
    for (u32 n=0;n<charCount_;++n) {
        const u32 i=(primaryChar_+n)%charCount_;
        if (chars_[i].connection) { primaryChar_=i; break; }
    }

    lastHealthMs_=tickMs(); started_=true; return true;
}

bool FogTransport100::openMcp() noexcept {
    auto& f=fogApi100();
    if (!mcpAddress_[0]) return false;
    mcp_=f.ord10047(mcpAddress_.data(),6112,"GameServer->MCP");
    if (!mcp_) return false;

    f.ord10056(mcp_,&FogMcpPreDispatch100,nullptr);
    f.ord10052(mcp_);

    const u8 hello=0x02;
    if (!f.ord10050(mcp_,&hello,1)) {
        f.ord10048(mcp_); mcp_=nullptr; return false;
    }

    // VA 0x10008550 sends a one-byte framed MCP opcode 0x01 immediately
    // after the control hello. Incoming MCP 0x01 later elicits the 17-byte
    // opcode-0x02 registration record in FromMCP.cpp; they are distinct.
    const u8 registerRequest=0x01;
    if (!f.ord10049(mcp_,&registerRequest,1)) {
        f.ord10048(mcp_); mcp_=nullptr; return false;
    }

    auto& net=d2netApi100();
    if (net.ord10018) net.ord10018(&HackReportToMcp100);
    return true;
}

bool FogTransport100::openChar(u32 i,bool reconnect) noexcept {
    if (i>=charCount_ || !slotUsableAddress(chars_[i].address)) return false;
    auto& f=fogApi100(); auto& s=chars_[i];
    if (s.connection) f.ord10048(s.connection);

    char identity[64]{};
    std::snprintf(identity,sizeof(identity),"Game Server->Char Server #%u",(unsigned)i);
    s.connection=f.ord10047(s.address.data(),6113,identity);
    if (!s.connection) return false;
    f.ord10056(s.connection,nullptr,nullptr);

    if (reconnect) {
        // Original reconnect path uses Fog #10053 asynchronously and does NOT
        // send hello 0x05 until the worker handle has completed.
        s.reconnectThread=f.ord10053(s.connection);
        return s.reconnectThread!=nullptr;
    }

    f.ord10052(s.connection);
    const u8 hello=0x05;
    if (!f.ord10050(s.connection,&hello,1)) {
        f.ord10048(s.connection); s.connection=nullptr; return false;
    }
    return true;
}

void FogTransport100::stop() noexcept {
    auto& net=d2netApi100();
    // Original MCP shutdown clears D2Net #10018 before destroying the Fog link.
    if (net.ord10018) net.ord10018(nullptr);

    auto& f=fogApi100();
    if (f.ord10048) {
        if (mcp_) f.ord10048(mcp_);
        for (auto& s:chars_) if (s.connection) f.ord10048(s.connection);
    }
#if defined(_WIN32)
    for (auto& s:chars_) {
        if (s.reconnectThread) CloseHandle((HANDLE)s.reconnectThread);
    }
#endif
    mcp_=nullptr; mcpAddress_.fill(0);
    for (auto& s:chars_) { s.connection=nullptr; s.reconnectThread=nullptr; s.address.fill(0); }
    charCount_=0; primaryChar_=0; started_=false;
}

bool FogTransport100::sendMcp(const void* data,std::size_t size) noexcept {
    auto& f=fogApi100();
    if (!mcp_ || !f.ord10049 || size>0xFFFFu) return false;
    return f.ord10049(mcp_,data,(u32)size)!=0;
}

bool FogTransport100::sendCharServer(const void* data,std::size_t size) noexcept {
    auto& f=fogApi100();
    if (!f.ord10049 || !charCount_ || size>0xFFFFu) return false;
    u32 cur=primaryChar_%charCount_;
    for (u32 n=0;n<charCount_;++n) {
        auto& s=chars_[cur];
        if (s.connection && f.ord10049(s.connection,data,(u32)size)) { primaryChar_=cur; return true; }
        cur=(cur+1)%charCount_;
    }
    return false;
}

void FogTransport100::pollMcp() noexcept {
    auto& f=fogApi100(); if (!mcp_ || !f.ord10051) return;
    std::array<u8,0x2020> buf{};
    for (;;) {
        int n=f.ord10051(mcp_,buf.data(),(u32)buf.size());
        if (n<=0) break;
        DispatchFromMcp(buf.data(),(u32)n);
    }
}
void FogTransport100::pollChars() noexcept {
    auto& f=fogApi100(); if (!f.ord10051) return;
    std::array<u8,0x1020> buf{};
    for (u32 i=0;i<charCount_;++i) {
        if (!chars_[i].connection) continue;
        const int n=f.ord10051(chars_[i].connection,buf.data(),(u32)buf.size());
        if (n>0) DispatchFromCharServer100(buf.data(),(u32)n);
    }
}

void FogTransport100::healthCheck() noexcept {
    const u32 now=tickMs(); if ((u32)(now-lastHealthMs_)<60000u) return; lastHealthMs_=now;
    auto& f=fogApi100(); if (!f.ord10058) return;

    if (mcp_ && !f.ord10058(mcp_)) {
        f.ord10048(mcp_); mcp_=nullptr; openMcp();
    }

    for (u32 i=0;i<charCount_;++i) {
        auto& s=chars_[i];
        if (!slotUsableAddress(s.address)) continue;

        if (s.connection && f.ord10058(s.connection)) {
            // Stock health code sends opcode 0x08 through the same primary-
            // based round-robin CharServ sender used by normal outbound data,
            // then preserves the successful slot as the new primary.
            const u8 heartbeat=0x08;
            sendCharServer(&heartbeat,1);
            continue;
        }

        if (s.reconnectThread) {
            bool completed=false;
#if defined(_WIN32)
            const DWORD wait=WaitForSingleObject((HANDLE)s.reconnectThread,0);
            completed=(wait!=WAIT_TIMEOUT);
            if (completed) { CloseHandle((HANDLE)s.reconnectThread); s.reconnectThread=nullptr; }
#else
            // Portable harness has no HANDLE semantics; ActiveTransport is the
            // observable completion signal available to source-equivalent tests.
            completed=s.connection && f.ord10058(s.connection)!=nullptr;
            if (completed) s.reconnectThread=nullptr;
#endif
            if (!completed) continue;
            if (s.connection) {
                const u8 hello=0x05;
                f.ord10050(s.connection,&hello,1);
            }
            continue;
        }

        openChar(i,true);
    }
}
void FogTransport100::serviceTick() noexcept { if(started_){ pollChars(); pollMcp(); healthCheck(); } }

} // namespace d2server100
