#include "CharServBridge100.h"
#include "D2Server100.h"
namespace d2server100 {
int DeliverDatabaseCharacterReply100(const u8* packet,u32 length) noexcept {
    if (!packet || length<0x11 || !bindings().d2game.ord10007) return 0;
    const u32 key=read_u32(packet+0x05);
    const u16 chunk=read_u16(packet+0x09);
    const u16 total=read_u16(packet+0x0B);
    const u32 mode=read_u32(packet+0x0D);
    if (length < 0x11u + chunk) return 0;
    return bindings().d2game.ord10007(key,packet+0x11,chunk,total,mode);
}
int DispatchFromCharServer100(const u8* packet,u32 length) noexcept {
    if (!packet || !length || packet[0]>=9) return 0;
    switch(packet[0]) {
        case 3: return DeliverDatabaseCharacterReply100(packet,length);
        case 5: return 1;
        case 8: return 1;
        default: return 0;
    }
}
}
