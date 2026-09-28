#include "PlayerToken.h"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace d2server100;
static u32 fakeTick = 1000;
static u32 Tick() { return fakeTick; }
int main() {
    PlayerTokenStore store(&Tick);
    assert(store.upsert("Hero", "Account", 0xAABBCCDD, 0x11223344, 77));
    char account[16]{}; u32 saveToken = 0;
    assert(store.consume("Hero", 0x11223344, 77, account, &saveToken));
    assert(std::strncmp(account, "Account", 7) == 0);
    assert(saveToken == 0xAABBCCDD);
    assert(!store.consume("Hero", 0x11223344, 77, account, &saveToken));

    assert(store.upsert("Old", "Acct2", 3, 1, 2));
    fakeTick += PlayerTokenStore::kLifetimeMs + 1;
    assert(store.upsert("New", "Acct3", 4, 5, 6)); // purges Old
    assert(!store.consume("Old", 1, 2, account, &saveToken));

    // Binary consumes a character-name match even when token validation fails.
    assert(store.upsert("OneShot", "Acct4", 7, 8, 9));
    assert(!store.consume("OneShot", 999, 9, account, &saveToken));
    assert(!store.consume("OneShot", 8, 9, account, &saveToken));

    // Retail uses Storm #501/SStrCopy(max=16): at most 15 visible bytes and
    // a guaranteed NUL, even when the input is longer than the field.
    assert(store.upsert("12345678901234567890", "ABCDEFGHIJKLMNO123", 10, 11, 12));
    const auto* rec=store.head();
    assert(rec && std::strlen(rec->characterName)==15);
    assert(std::strncmp(rec->characterName,"123456789012345",15)==0);
    char truncatedAccount[16]{}; u32 truncatedSave=0;
    assert(store.consume("123456789012345",11,12,truncatedAccount,&truncatedSave));
    assert(truncatedAccount[15]=='\0');
    assert(std::strncmp(truncatedAccount,"ABCDEFGHIJKLMNO",15)==0);
    std::cout << "PlayerToken reconstruction tests passed\n";
}
