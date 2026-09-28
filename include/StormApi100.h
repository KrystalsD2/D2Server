#pragma once
#include "Compat.h"

namespace d2server100 {

struct StormApi100 {
    using AllocFn = void* (D2_STDCALL *)(u32 size, const char* typeName, s32 pool, u32 flags);
    using FreeFn  = int   (D2_STDCALL *)(void* ptr, const char* typeName, s32 pool, u32 flags);

    void* module = nullptr;
    AllocFn alloc401 = nullptr;
    FreeFn free403 = nullptr;

    bool resolve(const char* moduleName = "Storm.dll") noexcept;
    void release() noexcept;
    bool ready() const noexcept { return alloc401 && free403; }
};

StormApi100& stormApi100() noexcept;

} // namespace d2server100
