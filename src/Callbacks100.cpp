#include "Callbacks100.h"
#include "D2Server100.h"
#include "FogApi100.h"
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>

namespace d2server100 {
namespace {

std::array<u8,0x88> g_ladderState{};

inline u32 rol1(u32 v) noexcept { return (v << 1) | (v >> 31); }

bool sendMcp(const void* data,std::size_t size) noexcept {
    return bindings().sendToMcp && bindings().sendToMcp(data,size);
}
bool sendChar(const void* data,std::size_t size) noexcept {
    return bindings().sendToCharServer && bindings().sendToCharServer(data,size);
}

void copyFixed(char* dst,std::size_t capacity,const char* src) noexcept {
    if (!dst || !capacity) return;
    std::memset(dst,0,capacity);
    if (!src) return;
    const std::size_t n=bounded_strlen(src,capacity-1);
    std::memcpy(dst,src,n);
}

u32 readLadderDword(std::size_t off) noexcept {
    if (off+4>g_ladderState.size()) return 0;
    return read_u32(g_ladderState.data()+off);
}

bool expAtLeast(u32 low,u32 high,u32 oldLow,u32 oldHigh) noexcept {
    return high>oldHigh || (high==oldHigh && low>=oldLow);
}

u32 packClassLevelStatus(u32 charClass,u32 charLevel,u32 status) noexcept {
    u32 packed=(charClass & 7u) | ((charLevel & 0xFFu)<<16);
    if (status & 0x04u) {
        packed |= 0x20u;
        if (status & 0x08u) packed |= 0x10u;
    }
    const u32 title=status & 0x1F00u;
    if (title>=0x0400u) {
        packed &= ~0x1F00u;
        packed |= title;
    }
    return packed;
}

template<class F> GenericCallback100 asGeneric(F f) noexcept {
    return reinterpret_cast<GenericCallback100>(f);
}

} // namespace

u32 SaveChecksum100(const void* data,u32 size) noexcept {
    if (!data) return 0;
    const auto* p=static_cast<const u8*>(data);
    u32 value=0;
    for (u32 i=0;i<size;++i) value=rol1(value)+p[i];
    return value;
}

void UpdateLadderStateBlob100(const u8* data,std::size_t size) noexcept {
    if (!data || size!=g_ladderState.size()) return;
    std::memcpy(g_ladderState.data(),data,g_ladderState.size());
}
const u8* LadderStateBlob100() noexcept { return g_ladderState.data(); }

void D2_FASTCALL Callback_CloseGame(u32 gameId) {
    u8 packet[5]{};
    packet[0]=0x05;
    write_u32(packet+1,gameId);
    sendMcp(packet,sizeof(packet));
}

void D2_FASTCALL Callback_LeaveGame(u32 gameId,u32 charClass,
                                    u32 charLevel,u32 expLow,u32 expHigh,
                                    u32 charStatus,const char* characterName,
                                    const char* characterPortrait) {
    // The wrapper at 0x10009420 unlocks stack argument #5 (the character
    // name) before forwarding all eight logical arguments to 0x100088C0.
    Callback_UnlockDatabaseCharacter(characterName);

    if (!characterName) characterName="";
    if (!characterPortrait) characterPortrait="";
    const std::size_t nameLen=bounded_strlen(characterName,0x1FF);
    const std::size_t portraitLen=bounded_strlen(characterPortrait,0x1FF);
    if (nameLen>=0x1FF || portraitLen>=0x1FF) return;

    // Exact fixed prefix reconstructed from VA 0x100088C0.
    std::array<u8,0x216> packet{};
    packet[0]=0x08;
    write_u32(packet.data()+1,gameId);
    write_u32(packet.data()+5,packClassLevelStatus(charClass,charLevel,charStatus));
    write_u16(packet.data()+9,static_cast<u16>(charLevel));
    write_u32(packet.data()+11,expLow);
    write_u32(packet.data()+15,expHigh);
    write_u16(packet.data()+19,static_cast<u16>(charStatus));
    std::memcpy(packet.data()+21,characterName,nameLen+1);
    std::memcpy(packet.data()+22+nameLen,characterPortrait,portraitLen+1);
    sendMcp(packet.data(),22+nameLen+portraitLen+1);
}

int D2_FASTCALL Callback_GetDatabaseCharacter(const char* characterName,u32 requestTag) {
    if (!characterName) return 0;
    const std::size_t n=bounded_strlen(characterName,255);
    if (n>=255) return 0;
    std::array<u8,266> packet{};
    packet[0]=0x03;
    write_u32(packet.data()+1,1);          // exact original constant
    write_u32(packet.data()+5,requestTag);
    std::memcpy(packet.data()+9,characterName,n+1);
    return sendChar(packet.data(),10+n) ? 1 : 0;
}

int D2_FASTCALL Callback_SaveDatabaseCharacter(const char* characterName,
                                                const char* accountName,
                                                const void* saveData,
                                                u32 saveSize,
                                                u32 playerData) {
    if (!characterName || !accountName || !saveData || saveSize<2) return 0;
    if (read_u16(static_cast<const u8*>(saveData)) != static_cast<u16>(saveSize)) {
        logLine("D2Server100: SaveDatabaseCharacter size header mismatch");
        return 0;
    }
    const std::size_t charLen=bounded_strlen(characterName,255);
    const std::size_t accountLen=bounded_strlen(accountName,255);
    if (charLen>=255 || accountLen>=255) return 0;

    const std::size_t total=13+(charLen+1)+(accountLen+1)+saveSize;
    std::vector<u8> packet(total,0);
    packet[0]=0x02;

    u32 transaction=0;
    auto& fog=fogApi100();
    if ((fog.ready() || fog.resolve()) && fog.ord10036) transaction=fog.ord10036();
    write_u32(packet.data()+1,transaction);
    write_u32(packet.data()+5,playerData);
    write_u32(packet.data()+9,SaveChecksum100(saveData,saveSize));
    std::size_t off=13;
    std::memcpy(packet.data()+off,characterName,charLen+1); off+=charLen+1;
    std::memcpy(packet.data()+off,accountName,accountLen+1); off+=accountLen+1;
    std::memcpy(packet.data()+off,saveData,saveSize);
    return sendChar(packet.data(),packet.size()) ? 1 : 0;
}

void D2_CDECL Callback_ServerLogMessage(u32 count,const char* format,...) {
    (void)count;
    if (!format) return;
    char text[512]{};
    va_list ap;
    va_start(ap,format);
#if defined(_MSC_VER)
    _vsnprintf(text,sizeof(text)-1,format,ap);
#else
    std::vsnprintf(text,sizeof(text),format,ap);
#endif
    va_end(ap);
    text[sizeof(text)-1]='\0';
    logLine(text);
}

void D2_FASTCALL Callback_EnterGame(u32 gameId,const char* characterName,
                                   u32 charClass,u32 charLevel,u32 zero) {
    if (!characterName) return;
    const std::size_t n=bounded_strlen(characterName,0x1FF);
    if (n>=0x1FF) return;
    std::array<u8,0x20C> packet{};
    packet[0]=0x0A;
    write_u32(packet.data()+1,gameId);
    write_u16(packet.data()+5,static_cast<u16>(charClass));
    write_u16(packet.data()+7,static_cast<u16>(charLevel));
    write_u16(packet.data()+9,static_cast<u16>(zero));
    std::memcpy(packet.data()+11,characterName,n+1);
    sendMcp(packet.data(),12+n);
}

int D2_FASTCALL Callback_FindPlayerToken(const char* characterName,u32 token,u32 gameId,
                                         char* outAccountName,u32* outCharacterSaveToken) {
    if (!outAccountName || !outCharacterSaveToken) return 0;
    return playerTokens().consume(characterName,token,gameId,
                                  outAccountName,outCharacterSaveToken) ? 1 : 0;
}

int D2_FASTCALL Callback_SaveDatabaseGuild(const char* guildTag,const void* data,u32 size) {
    if (!size) return 1;
    if (!guildTag || !data) return 0;
    const auto* src=static_cast<const u8*>(data);
    u32 offset=0;
    while (offset<size) {
        const u16 chunk=static_cast<u16>((size-offset)>0x190u ? 0x190u : (size-offset));
        std::array<u8,0x19B> packet{};
        packet[0]=0x0C;
        copyFixed(reinterpret_cast<char*>(packet.data()+1),4,guildTag);
        write_u16(packet.data()+5,static_cast<u16>(offset));
        write_u16(packet.data()+7,chunk);
        write_u16(packet.data()+9,static_cast<u16>(size));
        std::memcpy(packet.data()+11,src+offset,chunk);
        if (!sendMcp(packet.data(),11+chunk)) return 0;
        offset+=chunk;
    }
    return 1;
}

int D2_FASTCALL Callback_UnlockDatabaseCharacter(const char* characterName) {
    if (!characterName) return 0;
    const std::size_t n=bounded_strlen(characterName,255);
    if (n>=255) return 0;
    std::array<u8,258> packet{};
    packet[0]=0x07;
    std::memcpy(packet.data()+1,characterName,n+1);
    return sendChar(packet.data(),2+n) ? 1 : 0;
}

void D2_FASTCALL Callback09(const char* tag,u32 type,const char* text,u32 value) {
    std::array<u8,0x1A> packet{};
    packet[0]=0x0E;
    packet[1]=static_cast<u8>(type);
    copyFixed(reinterpret_cast<char*>(packet.data()+2),4,tag);
    write_u32(packet.data()+6,value);
    copyFixed(reinterpret_cast<char*>(packet.data()+10),16,text);
    sendMcp(packet.data(),packet.size());
}

int D2_FASTCALL Callback_UpdateCharacterLadder(const char* characterName,u32 charClass,
                                                u32 charLevel,u32 expLow,u32 expHigh,
                                                u32 charStatus) {
    if (!characterName || charClass>=5) return 0;

    // Exact layout inside MCP opcode-0F state blob copied to 0x100294F8.
    const bool branch=(charStatus & 0x04u)!=0;
    const std::size_t globalOff=branch ? 0x00 : 0x48;
    const std::size_t classOff=(branch ? 0x08 : 0x50)+(charClass*8u);
    if (!expAtLeast(expLow,expHigh,readLadderDword(globalOff),readLadderDword(globalOff+4)) &&
        !expAtLeast(expLow,expHigh,readLadderDword(classOff),readLadderDword(classOff+4))) {
        return 0;
    }

    const std::size_t n=bounded_strlen(characterName,0x1FF);
    if (n>=0x1FF) return 0;
    std::array<u8,0x210> packet{};
    packet[0]=0x0F;
    write_u32(packet.data()+1,packClassLevelStatus(charClass,charLevel,charStatus));
    write_u32(packet.data()+5,expLow);
    write_u32(packet.data()+9,expHigh);
    write_u16(packet.data()+13,static_cast<u16>(charStatus));
    std::memcpy(packet.data()+15,characterName,n+1);
    return sendMcp(packet.data(),16+n) ? 1 : 0;
}

void D2_FASTCALL Callback_UpdateGameInformation(u32 gameId,const char* characterName,
                                                u32 charClass,u32 charLevel) {
    if (!characterName || charClass>=5) return;
    const std::size_t n=bounded_strlen(characterName,0x1FF);
    if (n>=0x1FF) return;
    std::array<u8,0x208> packet{};
    packet[0]=0x10;
    write_u32(packet.data()+1,gameId);
    packet[5]=static_cast<u8>(charClass);
    packet[6]=static_cast<u8>(charLevel);
    std::memcpy(packet.data()+7,characterName,n+1);
    sendMcp(packet.data(),8+n);
}

const EventCallbackTable100& eventCallbackTable100() noexcept {
    static const EventCallbackTable100 table{{
        asGeneric(&Callback_CloseGame),
        asGeneric(&Callback_LeaveGame),
        asGeneric(&Callback_GetDatabaseCharacter),
        asGeneric(&Callback_SaveDatabaseCharacter),
        asGeneric(&Callback_ServerLogMessage),
        asGeneric(&Callback_EnterGame),
        asGeneric(&Callback_FindPlayerToken),
        asGeneric(&Callback_SaveDatabaseGuild),
        asGeneric(&Callback_UnlockDatabaseCharacter),
        asGeneric(&Callback09),
        asGeneric(&Callback_UpdateCharacterLadder),
        asGeneric(&Callback_UpdateGameInformation)
    }};
    return table;
}

} // namespace d2server100
