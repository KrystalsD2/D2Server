# D2Server100 Reconstructed v0.3 - behavioral parity audit

This project aims for source-equivalent behavior, not binary-identical or line-for-line source recovery.

## High-confidence / implemented engine-facing behavior

- PE exports 10001, 10002, 10003 / QueryInterface shape.
- early start + 20-entry MCP handler interface.
- MCP dispatcher and CREATEGAME / JOINGAME parsing and replies.
- PlayerToken structure, list behavior, lookup/consume semantics and 120-second expiry.
- exact 12-entry 1.00 D2Game callback ABI.
- callback packet families: CloseGame, LeaveGame, Get/Save DB character, EnterGame, Save Guild, Unlock DB, early callback09, ladder update, game-info update.
- original save checksum loop.
- Fog #10036 transaction source for SaveDatabaseCharacter.
- MCP opcode 0x0F ladder state blob and its link to UpdateCharacterLadder.
- exact 0x68 GameDataHash outer layout and 12-byte list/bucket headers.
- SGAMEDATA allocation/free hooks using Storm #401/#403 and 0x3060 base allocation.
- D2Game #10046 runtime init, #10023 callback install, #10002 game-data install, #10039 post-install init.
- D2Game #10047 CREATEGAME call boundary.
- D2Game #10007 five-argument database-character delivery bridge.
- beginning of original shutdown sequence (#10040 and #10050).

## Partially reconstructed

- MCP handlers 0x01 and 0x02: fixed records reconstructed, but not every dynamic server-status field.
- MCP handler 0x03: known to enter archive/enumeration status logic, deeper semantics not yet reproduced.
- handlers 0x12 / 0x13: validation and immediate call boundary known, downstream service semantics incomplete.
- exact retry/round-robin policy for multiple CharServ connections. RuntimeBindings100 intentionally abstracts transport.
- all original assert/fatal/error-display behavior; source generally fails safely instead.

## Major original application-shell behavior still missing

- Bnclient.dll QueryInterface and complete MCP connection establishment/reconnect loop.
- complete CharServ connection pool, reconnect, selection and fatal-loss behavior.
- D2Net client accept/network-service initialization and original worker/tick loops.
- realms.ini parser and realm/accept/character-server configuration objects.
- WinMain thread/message-loop orchestration and remote shutdown handling.
- Windows status GUI, Display.cpp, performance graphing/logging and console commands.
- Archive.cpp enumeration/status integration used by the server shell.
- exact complete teardown order for every owned resource and connection.

## Parity estimate (not a line-count estimate)

Ranges are intentionally broad because not all original subsystems have equal importance.

- D2Game-facing contract mapped: about 85-90%.
- D2Game-facing contract implemented in v0.3: about 75-85%.
- closed-realm game lifecycle/control behavior implemented or directly bridged: about 65-75%.
- full original D2Server application behavior, including transports, reconnects, INI, GUI and service shell: about 50-60%.

For a future RetroD2 1.06b adapter behind existing PvPGN/D2GS, much of the missing original shell is intentionally unnecessary. The useful engine-adapter knowledge is therefore significantly closer to completion than a literal 1.00 drop-in recreation.

## What would justify calling it behaviorally equal

A 1.00 build must be tested successfully through:

1. DLL load and QueryInterface.
2. original-equivalent service/bootstrap order.
3. MCP registration and control traffic.
4. CREATEGAME and repeated game allocation/removal.
5. JOINGAME and PlayerToken validation.
6. client transport / D2Net admission.
7. character load through CharServ -> D2Game #10007.
8. EnterGame and sustained gameplay.
9. SaveDatabaseCharacter, LeaveGame and Unlock.
10. CloseGame and clean SGAMEDATA removal.
11. many repeated create/join/save/leave/destroy cycles.
12. connection loss/reconnect and clean process shutdown.

Until those pass, the source should be described as a reconstruction candidate rather than a drop-in equivalent.

## Next high-value target found during this pass

The original WinMain D2Net startup calls are now isolated. D2Net #10023, #10026 and #10003 occur consecutively at D2Server 0x10009F5C..0x10009F6E; D2Net #10004 appears on the shutdown side at 0x1000A13C. See `D2NET_1_00_XREFS.md`. This is the clearest next step toward a literal live 1.00 drop-in replacement.
