#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <array>

#if defined(_MSC_VER) && defined(_M_IX86)
#  define D2_FASTCALL __fastcall
#  define D2_STDCALL __stdcall
#  define D2_CDECL __cdecl
#  define D2_EXPORT extern "C" __declspec(dllexport)
#else
#  define D2_FASTCALL
#  define D2_STDCALL
#  define D2_CDECL
#  define D2_EXPORT extern "C"
#endif

namespace d2server100 {
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using s32 = std::int32_t;

inline u16 read_u16(const u8* p) noexcept { u16 v{}; std::memcpy(&v,p,sizeof(v)); return v; }
inline u32 read_u32(const u8* p) noexcept { u32 v{}; std::memcpy(&v,p,sizeof(v)); return v; }
inline void write_u16(u8* p,u16 v) noexcept { std::memcpy(p,&v,sizeof(v)); }
inline void write_u32(u8* p,u32 v) noexcept { std::memcpy(p,&v,sizeof(v)); }
inline std::size_t bounded_strlen(const char* s,std::size_t max) noexcept {
    if (!s) return 0;
    std::size_t n=0;
    while (n<max && s[n]) ++n;
    return n;
}
inline bool ascii_iequal(const char* a,const char* b) noexcept {
    if (!a || !b) return false;
    for (;;) {
        unsigned char ca=(unsigned char)*a++, cb=(unsigned char)*b++;
        if (ca>='A'&&ca<='Z') ca=(unsigned char)(ca+('a'-'A'));
        if (cb>='A'&&cb<='Z') cb=(unsigned char)(cb+('a'-'A'));
        if (ca!=cb) return false;
        if (!ca) return true;
    }
}
}
