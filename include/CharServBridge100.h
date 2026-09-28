#pragma once
#include "Compat.h"
namespace d2server100 {
int DeliverDatabaseCharacterReply100(const u8* packet,u32 length) noexcept;
int DispatchFromCharServer100(const u8* packet,u32 length) noexcept;
}
