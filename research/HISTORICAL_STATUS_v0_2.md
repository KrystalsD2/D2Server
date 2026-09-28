# v0.2 reconstruction status

## Byte-verified engine boundary
- D2Game #10046 RVA 0x4C00: no-argument runtime initialization, returns 1 on normal path.
- D2Game #10023 RVA 0x4E10: stores one callback-table pointer and returns with `ret 4`.
- D2Game #10002 RVA 0x4DB0: requires two non-null pointers, stores the first game-data pointer, `ret 8`.
- D2Game #10007 RVA 0x56D0: `ret 0x14`; D2Server's only call pushes values from response +05, +11, +09, +0B, +0D.
- D2Game #10047 RVA 0x5090: CREATEGAME, `ret 0x20`.

## Original 12 callback pointers at D2Server VA 0x10024A90
Trampolines resolve to bodies at 0x10009500, 0x10009420, 0x10009470, 0x10009490,
0x10009310, 0x100093F0, 0x10008400, 0x10009520, 0x100093D0,
0x10009540, 0x10009570, 0x100095A0.

## GameDataHash
Original constructor body begins at D2Server VA 0x1000A240 and produces a 0x68-byte object.
D2Game #10047 uses offsets +1C, +24, +48, +4C and +50 directly, then calls `vtable+4`
during insertion. The final original vtable pointer is 0x1001F02C; a replacement must provide its own compatible implementation rather than pointing into the historical DLL.
