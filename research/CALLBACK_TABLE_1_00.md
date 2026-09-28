# Diablo II 1.00 D2Server -> D2Game callback table

Authoritative binary: D2Server.dll, May 26 2000, image base 0x10000000.
Table VA: 0x10024A90. Twelve x86 pointers (0x30 bytes).

| Slot | Public analytical name | Original wrapper/body | Proven x86 ABI | Primary outbound behavior |
|---:|---|---|---|---|
| 0 | CloseGame | 0x10009500 -> 0x10008780 | fastcall ECX only | MCP 0x05, 5 bytes |
| 1 | LeaveGame | 0x10009420 -> 0x100088C0 | fastcall + six stack args, RET 0x18 | unlock char, MCP 0x08 |
| 2 | GetDatabaseCharacter | 0x10009470 -> 0x10002210 | fastcall ECX/EDX | CharServ 0x03 |
| 3 | SaveDatabaseCharacter | 0x10009490 -> 0x10002030 | fastcall ECX/EDX + 3 stack, RET 0x0C | CharServ 0x02 |
| 4 | ServerLogMessage | 0x10009310 | cdecl/variadic family | formatting / display log |
| 5 | EnterGame | 0x100093F0 -> 0x100089E0 | fastcall ECX/EDX + 3 stack, RET 0x0C | MCP 0x0A |
| 6 | FindPlayerToken | 0x10008400 | fastcall + 3 stack | consumes 120s PlayerToken record |
| 7 | SaveDatabaseGuild | 0x10009520 -> 0x10008AE0 | fastcall ECX/EDX + 1 stack, RET 0x04 | MCP 0x0C, 400-byte chunks |
| 8 | UnlockDatabaseCharacter | 0x100093D0 -> 0x100024D0 | ECX only | CharServ 0x07 |
| 9 | Early callback 09 | 0x10009540 -> 0x10008C00 | fastcall ECX/EDX + 2 stack, RET 0x08 | MCP 0x0E, fixed 0x1A bytes |
| 10 | UpdateCharacterLadder | 0x10009570 -> 0x10008CA0 | fastcall ECX/EDX + 4 stack, RET 0x10 | MCP 0x0F |
| 11 | UpdateGameInformation | 0x100095A0 -> 0x10008E20 | fastcall ECX/EDX + 2 stack, RET 0x08 | MCP 0x10 |

## Important version differences

This table is not ABI-identical to the later 1.09d 16-callback EVENTCALLBACKTABLE. In particular:

- 1.00 has only 12 entries.
- 1.00 LeaveGame is the compact eight-logical-argument Classic form.
- 1.00 UpdateCharacterLadder has six arguments, with no later PlayerMark argument.
- slot 9 is live in 1.00 and emits opcode 0x0E; it later became a reserved callback.
- D2Game #10007 is a separate five-stack-argument database-character delivery ABI (RET 0x14).

## Ladder blob connection

MCP handler 0x0F at D2Server 0x100059D0 copies exactly 0x88 bytes to the global region beginning 0x100294F8. UpdateCharacterLadder at 0x10008CA0 reads its comparison thresholds from that same region. v0.3 therefore models this as one named ladder-state blob rather than unrelated globals.
