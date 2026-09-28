#include "GameDataHash100.h"
#include "StormApi100.h"

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace d2server100 {
namespace {
constexpr char kSGameDataTypeName[] = ".?AUSGAMEDATA@@";

#if defined(_WIN32) && !defined(_WIN64) && defined(_MSC_VER) && defined(_M_IX86)

inline u32 ptr32(const void* p) noexcept {
    return static_cast<u32>(reinterpret_cast<std::uintptr_t>(p));
}
template<class F> inline u32 fn32(F p) noexcept {
    return static_cast<u32>(reinterpret_cast<std::uintptr_t>(p));
}

inline bool isRealObject(u32 p) noexcept {
    return static_cast<s32>(p) > 0;
}

void initExplicitList(ExplicitList100& list, u32 linkOffset) noexcept {
    list.linkOffset = linkOffset;
    list.tailLinkRef = ptr32(&list.tailLinkRef);
    list.headObject = ~list.tailLinkRef;
}

// Removes an object's two-dword embedded intrusive link from whichever
// ExplicitList currently owns it. This is the byte-level behavior of the
// helper reached by original D2Server 0x1000101E / 0x1000AD50.
void detachEmbeddedLink(void* object, u32 linkOffset) noexcept {
    if (!object) return;
    auto* link = reinterpret_cast<u32*>(reinterpret_cast<u8*>(object) + linkOffset);
    const u32 prevRef = link[0];
    const u32 next = link[1];
    if (!prevRef) return;

    u32 targetPrevRef;
    if (static_cast<s32>(next) < 0) {
        targetPrevRef = ~next;
    } else {
        targetPrevRef = next + linkOffset;
    }

    *reinterpret_cast<u32*>(static_cast<std::uintptr_t>(targetPrevRef)) = prevRef;
    *reinterpret_cast<u32*>(static_cast<std::uintptr_t>(prevRef + 4)) = next;
    link[0] = 0;
    link[1] = 0;
}

void insertAtHead(ExplicitList100* list, void* object) noexcept {
    if (!list || !object) return;
    const u32 offset = list->linkOffset;
    auto* link = reinterpret_cast<u32*>(reinterpret_cast<u8*>(object) + offset);

    // Original generic insertion first detaches the supplied link if needed.
    if (link[0]) detachEmbeddedLink(object, offset);

    const u32 anchor = ptr32(&list->tailLinkRef);
    const u32 oldHead = list->headObject;
    const u32 linkAddress = ptr32(link);

    link[0] = anchor;
    link[1] = oldHead;

    // Update old head's prev-ref, or the encoded end anchor when the list was empty.
    const u32 backRef = static_cast<s32>(oldHead) < 0 ? ~oldHead : oldHead + offset;
    *reinterpret_cast<u32*>(static_cast<std::uintptr_t>(backRef)) = linkAddress;

    list->headObject = ptr32(object);
}

void D2_FASTCALL HashVDestroyNode(GameDataHashLayout100*, void*, void* node) {
    if (!node) return;
    // SGAMEDATA participates in master list at +0x0C and bucket list at +0x04.
    detachEmbeddedLink(node, 0x0C);
    detachEmbeddedLink(node, 0x04);
    auto& storm = stormApi100();
    if (storm.ready()) storm.free403(node, kSGameDataTypeName, -2, 0);
}

void* D2_FASTCALL HashVAllocateNode(GameDataHashLayout100*, void*,
                                    GameDataBucket100* bucket,
                                    u32 extraBytes, u32 flags) {
    auto& storm = stormApi100();
    if (!storm.ready() || !bucket) return nullptr;

    // Exact original generic allocator behavior for the CREATEGAME call site:
    // size = 0x3060 + arg2; flags = lowByte(arg3) | 8.
    const u32 bytes = GameDataHash100::kSGameDataBytes + extraBytes;
    const u32 allocFlags = (flags & 0xFFu) | 0x08u;
    void* node = storm.alloc401(bytes, kSGameDataTypeName, -2, allocFlags);
    if (!node) return nullptr;

    auto* d = reinterpret_cast<u32*>(node);
    d[1] = 0; // +04 bucket prev-ref
    d[2] = 0; // +08 bucket next/head chain
    d[3] = 0; // +0C master prev-ref
    d[4] = 0; // +10 master next
    insertAtHead(bucket, node);
    return node;
}

// D2Game does not own the hash object and normal engine paths do not delete it.
// Keep this deleting-destructor slot ABI-compatible but never free our static wrapper.
GameDataHashLayout100* D2_FASTCALL HashVDeletingDestructor(GameDataHashLayout100* self,
                                                            void*, u32 /*flags*/) {
    return self;
}

void D2_FASTCALL HashVClear(GameDataHashLayout100* self, void*) {
    if (!self || !self->buckets) return;
    auto* buckets = reinterpret_cast<GameDataBucket100*>(static_cast<std::uintptr_t>(self->buckets));

    for (u32 i=0; i<self->bucketCount; ++i) {
        while (isRealObject(buckets[i].headObject)) {
            void* node = reinterpret_cast<void*>(static_cast<std::uintptr_t>(buckets[i].headObject));
            HashVDestroyNode(self, nullptr, node);
        }
        initExplicitList(buckets[i], 4);
    }
    initExplicitList(self->master, 0x0C);
    self->collisionCounter = 0;
}

u32 g_hashVtable[4] = {0,0,0,0};

bool installVtable(GameDataHashLayout100* self) noexcept {
    if (!self) return false;
    g_hashVtable[0] = fn32(&HashVDestroyNode);
    g_hashVtable[1] = fn32(&HashVAllocateNode);
    g_hashVtable[2] = fn32(&HashVDeletingDestructor);
    g_hashVtable[3] = fn32(&HashVClear);
    self->vtable = ptr32(g_hashVtable);
    return self->vtable != 0;
}

#endif
} // namespace

GameDataHash100::GameDataHash100() noexcept {
    std::memset(&layout_,0,sizeof(layout_));
    std::memset(buckets_.data(),0,sizeof(buckets_));

    layout_.collisionCounter = 0;
    layout_.capacity = 4;
    layout_.bucketCount = 4;
    layout_.field20 = 0;
    layout_.hashMask = 3;
    layout_.auxStride = 0x10;
    layout_.keyMeta = 0;
    layout_.wrapped = 0;

#if defined(_WIN32) && !defined(_WIN64) && defined(_MSC_VER) && defined(_M_IX86)
    initExplicitList(layout_.master, 0x0C);
    for (auto& b : buckets_) initExplicitList(b,4);
    // Constructor creates aux A as a generic list, then sets its link offset to 4.
    initExplicitList(layout_.auxListA,4);
    initExplicitList(layout_.auxListB,0x10);

    layout_.buckets = ptr32(buckets_.data());
    layout_.nextKey = static_cast<u32>((~reinterpret_cast<std::uintptr_t>(&layout_)) & 0x0FFFFFFFu);
    auto* cs = reinterpret_cast<CRITICAL_SECTION*>(layout_.criticalSection);
    InitializeCriticalSection(cs);
    lockInitialized_ = true;
#else
    // Portable analysis/test build: pointer-bearing fields intentionally remain
    // non-live. The byte-exact Win32 object is installed only in an x86 MSVC DLL.
    layout_.master.linkOffset = 0x0C;
    layout_.auxListA.linkOffset = 4;
    layout_.auxListB.linkOffset = 0x10;
    for (auto& b : buckets_) b.linkOffset = 4;
    layout_.nextKey = 1;
#endif
}

GameDataHash100::~GameDataHash100() {
#if defined(_WIN32) && !defined(_WIN64) && defined(_MSC_VER) && defined(_M_IX86)
    if (insertionMethodImplemented_) HashVClear(&layout_,nullptr);
    if (lockInitialized_) DeleteCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(layout_.criticalSection));
#endif
}

bool GameDataHash100::initializeRuntime() noexcept {
#if defined(_WIN32) && !defined(_WIN64) && defined(_MSC_VER) && defined(_M_IX86)
    if (insertionMethodImplemented_) return true;
    if (!stormApi100().resolve()) return false;
    if (!installVtable(&layout_)) return false;
    insertionMethodImplemented_ = true;
    return true;
#else
    return false;
#endif
}

} // namespace d2server100
