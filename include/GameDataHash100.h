#pragma once
#include "Compat.h"
#include <array>

namespace d2server100 {

// Intrusive explicit-list header used repeatedly by Blizzard's TSExplicitList.
// Its two link fields use an encoded end sentinel (~addressOfAnchor).
#pragma pack(push,1)
struct ExplicitList100 {
    u32 linkOffset;        // +00: offset of embedded link in member object
    u32 tailLinkRef;       // +04: pointer to tail object's link, or &this+4 when empty
    u32 headObject;        // +08: head object pointer, or encoded end sentinel when empty
};
#pragma pack(pop)
static_assert(sizeof(ExplicitList100)==0x0C, "1.00 explicit list header is 12 bytes");

#pragma pack(push,1)
struct GameDataHashLayout100 {
    u32 vtable;                 // +00 replacement vtable (original 0x1001F02C)
    ExplicitList100 master;     // +04; SGAMEDATA link offset 0x0C
    u32 collisionCounter;       // +10

    // Original TSArray/TSHashObjectChunk vector metadata.
    u32 capacity;               // +14 = 4 initially
    u32 bucketCount;            // +18 = 4 initially
    u32 buckets;                // +1C -> GameDataBucket100[4]
    u32 field20;                // +20 = 0 initially
    u32 hashMask;               // +24 = 3 initially

    ExplicitList100 auxListA;   // +28; constructor ultimately uses offset 4
    u32 auxStride;              // +34 = 0x10
    ExplicitList100 auxListB;   // +38; link offset 0x10
    u32 keyMeta;                // +44; zero-initialized in original global object

    u32 nextKey;                // +48 = (~this)&0x0FFFFFFF
    u32 wrapped;                // +4C = 0
    u8  criticalSection[0x18];  // +50 Win32 CRITICAL_SECTION
};
#pragma pack(pop)
static_assert(sizeof(GameDataHashLayout100)==0x68, "1.00 game-data container is 0x68 bytes");

using GameDataBucket100 = ExplicitList100;
static_assert(sizeof(GameDataBucket100)==0x0C, "1.00 initial bucket is 12 bytes");

class GameDataHash100 {
public:
    static constexpr u32 kOriginalFinalVtableVA = 0x1001F02C;
    static constexpr u32 kSGameDataBytes = 0x3060;

    GameDataHash100() noexcept;
    ~GameDataHash100();
    GameDataHash100(const GameDataHash100&)=delete;
    GameDataHash100& operator=(const GameDataHash100&)=delete;

    GameDataHashLayout100* layout() noexcept { return &layout_; }
    const GameDataHashLayout100* layout() const noexcept { return &layout_; }

    // Resolves Storm #401/#403 and installs this reconstruction's x86 vtable.
    // Must succeed before D2Game #10002/#10047 are allowed to use the object.
    bool initializeRuntime() noexcept;
    bool readyForD2GameCreate() const noexcept { return insertionMethodImplemented_; }

private:
    GameDataHashLayout100 layout_{};
    std::array<GameDataBucket100,4> buckets_{};
    bool lockInitialized_ = false;
    bool insertionMethodImplemented_ = false;
};

} // namespace d2server100
