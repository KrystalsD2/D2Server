# PlayerToken fixed-field copy parity — Storm #501

## Retail evidence

The Diablo II 1.00 `D2Server.dll` PlayerToken path does not copy sixteen bytes blindly from the caller's character/account C strings.

At the retail PlayerToken insertion/update path (including the calls around VA `0x1000831C`, `0x1000835D`, and `0x1000836D`), D2Server calls its Storm import thunk for **Storm ordinal #501** with a maximum length of **16**.

Disassembly of the matching May 26, 2000 `Storm.dll` shows that #501 is a bounded, NUL-aware string-copy routine (the historical `SStrCopy` family): it copies through the source terminator when present and guarantees termination at the destination limit rather than performing a fixed 16-byte source read.

## Reconstruction consequence

v0.4 therefore uses a 16-byte destination helper with these semantics:

- at most 15 source characters are copied;
- byte 15 is available for the terminating NUL;
- short source strings are never read beyond their terminator;
- an update need not zero every byte after the first NUL, matching the behavior expected from the retail bounded string copy;
- freshly allocated PlayerToken records are already zero-initialized.

This replaces the earlier reconstruction's fixed `memcpy(..., 16)`, which produced the right visible field for many ordinary inputs but was undefined behavior when passed a short separately allocated C string.

## Verification

The issue was discovered by an AddressSanitizer run of the portable test suite. After changing the implementation to the retail Storm #501 semantics and extending `token_tests` with overlength and termination cases:

- normal portable suite: **10/10 pass**;
- strict `-Wall -Wextra -Wpedantic` suite: **10/10 pass, zero compiler warnings**;
- AddressSanitizer + UndefinedBehaviorSanitizer suite: **10/10 pass**.

This is both a safety correction and a closer source-equivalent reconstruction of the retail binary.
