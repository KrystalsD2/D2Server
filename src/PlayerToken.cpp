#include "PlayerToken.h"
#include <chrono>
#include <new>

namespace d2server100 {
namespace {
// Retail D2Server does not memcpy these fixed fields.  Both copies at
// 0x1000831C/0x1000835D/0x1000836D call Storm #501 (SStrCopy) with max=16,
// which stops at NUL and always terminates the destination at the limit.
// Preserve that behavior without reading beyond a short C-string object.
void copyTokenField16(char (&destination)[16], const char* source) noexcept {
    if (!source) { destination[0]='\0'; return; }
    const std::size_t n=bounded_strlen(source,15);
    if (n) std::memcpy(destination,source,n);
    destination[n]='\0';
}
}

PlayerTokenStore::PlayerTokenStore(TickProvider tickProvider) noexcept
    : tick_(tickProvider ? tickProvider : &PlayerTokenStore::defaultTick) {}

PlayerTokenStore::~PlayerTokenStore() { clear(); }

u32 PlayerTokenStore::defaultTick() noexcept {
    using namespace std::chrono;
    return static_cast<u32>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

void PlayerTokenStore::purgeExpired(u32 now) noexcept {
    PlayerTokenRecord** link = &head_;
    while (*link) {
        PlayerTokenRecord* current = *link;
        // Unsigned subtraction deliberately mirrors the original GetTickCount arithmetic.
        if (static_cast<u32>(now - current->createdTick) > kLifetimeMs) {
            *link = current->next;
            std::free(current);
        } else {
            link = &current->next;
        }
    }
}

PlayerTokenRecord** PlayerTokenStore::findLinkByCharacter(const char* characterName) noexcept {
    PlayerTokenRecord** link = &head_;
    while (*link) {
        if (ascii_iequal((*link)->characterName, characterName))
            return link;
        link = &(*link)->next;
    }
    return nullptr;
}

bool PlayerTokenStore::upsert(const char* characterName,
                              const char* accountName,
                              u32 characterSaveToken,
                              u32 token,
                              u32 gameId) {
    if (!characterName || !accountName) return false;

    const u32 now = tick_();
    purgeExpired(now);

    if (PlayerTokenRecord** link = findLinkByCharacter(characterName)) {
        PlayerTokenRecord* record = *link;
        record->token = token;
        record->gameId = gameId;
        record->createdTick = tick_();
        copyTokenField16(record->accountName, accountName);
        record->characterSaveToken = characterSaveToken;
        return true;
    }

    auto* record = static_cast<PlayerTokenRecord*>(std::calloc(1, sizeof(PlayerTokenRecord)));
    if (!record) return false;

    record->token = token;
    record->gameId = gameId;
    record->createdTick = tick_();
    copyTokenField16(record->characterName, characterName);
    copyTokenField16(record->accountName, accountName);
    record->characterSaveToken = characterSaveToken;
    record->next = head_;
    head_ = record;
    return true;
}

bool PlayerTokenStore::consume(const char* characterName,
                               u32 token,
                               u32 gameId,
                               char outAccountName[16],
                               u32* outCharacterSaveToken) {
    if (!characterName || !outAccountName || !outCharacterSaveToken) return false;

    const u32 now = tick_();
    PlayerTokenRecord** link = findLinkByCharacter(characterName);
    if (!link) return false;

    PlayerTokenRecord* record = *link;
    *link = record->next; // original unlinks before validating token/game/age

    const bool valid = record->token == token &&
                       record->gameId == gameId &&
                       static_cast<u32>(now - record->createdTick) <= kLifetimeMs;

    std::memcpy(outAccountName, record->accountName, sizeof(record->accountName));
    *outCharacterSaveToken = record->characterSaveToken;
    std::free(record);
    return valid;
}

void PlayerTokenStore::removeCharacter(const char* characterName) {
    if (!characterName) return;
    if (PlayerTokenRecord** link = findLinkByCharacter(characterName)) {
        PlayerTokenRecord* record = *link;
        *link = record->next;
        std::free(record);
    }
}

void PlayerTokenStore::clear() noexcept {
    while (head_) {
        PlayerTokenRecord* next = head_->next;
        std::free(head_);
        head_ = next;
    }
}

} // namespace d2server100
