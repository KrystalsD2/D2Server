#pragma once
#include "Compat.h"

namespace d2server100 {

// Directly reconstructed from D2Server.dll 1.00, VA 0x10008250..0x1000852D.
// The original allocation size is exactly 0x34 bytes.
#pragma pack(push, 1)
struct PlayerTokenRecord {
    u32 token;                  // +0x00 -- compared by FindPlayerToken
    u32 gameId;                 // +0x04 -- compared by FindPlayerToken
    u32 createdTick;            // +0x08 -- GetTickCount(), expires after 120000 ms
    char characterName[16];     // +0x0C -- lookup key
    char accountName[16];       // +0x1C -- returned by FindPlayerToken
    u32 characterSaveToken;     // +0x2C -- returned through output pointer
    PlayerTokenRecord* next;    // +0x30
};
#pragma pack(pop)
#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(PlayerTokenRecord) == 0x34, "1.00 PlayerToken record must remain 0x34 bytes on Win32");
#endif

using TickProvider = u32 (*)();

class PlayerTokenStore {
public:
    explicit PlayerTokenStore(TickProvider tickProvider = nullptr) noexcept;
    ~PlayerTokenStore();

    PlayerTokenStore(const PlayerTokenStore&) = delete;
    PlayerTokenStore& operator=(const PlayerTokenStore&) = delete;

    // Reconstructed 0x10008250 semantics.
    // Original fastcall shape:
    //   ECX = characterName, EDX = accountName,
    //   stack = characterSaveToken, token, gameId.
    bool upsert(const char* characterName,
                const char* accountName,
                u32 characterSaveToken,
                u32 token,
                u32 gameId);

    // Reconstructed 0x10008400 semantics. Matching-by-name consumes the record
    // even when token/gameId/age validation later fails.
    bool consume(const char* characterName,
                 u32 token,
                 u32 gameId,
                 char outAccountName[16],
                 u32* outCharacterSaveToken);

    // Reconstructed 0x100084E0 semantics.
    void removeCharacter(const char* characterName);

    void clear() noexcept;
    const PlayerTokenRecord* head() const noexcept { return head_; }

    static constexpr u32 kLifetimeMs = 0x1D4C0; // 120000 ms, direct binary constant

private:
    static u32 defaultTick() noexcept;
    void purgeExpired(u32 now) noexcept;
    PlayerTokenRecord** findLinkByCharacter(const char* characterName) noexcept;

    PlayerTokenRecord* head_ = nullptr;
    TickProvider tick_ = nullptr;
};

} // namespace d2server100
