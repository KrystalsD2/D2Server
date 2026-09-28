# D2Server100-Reconstructed v0.4 - final source checkpoint

v0.4 is the completed **integrated service-core reconstruction candidate** for Diablo II 1.00 `D2Server.dll`. It was reconstructed against the exact retail May 26, 2000 D2Server/D2Game/D2Net/Fog/D2Common/D2Lang family. It is not Blizzard source and is not claimed to be binary-identical.

Target retail D2Server used for parity work:

- size: 184,320 bytes
- SHA-256: `ccaaeb7800376991482e74f4e0e2c01f16088a197d45d38f39eb16ee50c2513b`
- linked: 2000-05-26 19:15:40 UTC

## What the final parity pass closed

The final pass went beyond the pre-teardown WIP and resolved these remaining service-core questions directly from the retail binaries:

1. **True D2Net startup mapping**
   - `#10003(0,0)`
   - `#10026(GetMultiClient())`
   - `#10023(1)`
   - the retail caller ignores the return from #10003.

2. **Fog process initialization**
   - Fog `#10139` receives ECX=1 before the other service work;
   - Fog `#10019` receives the literal `D2SDebug` and initializes ErrorManager.

3. **Matched engine-global lifecycle**
   - startup: D2Lang `#10000(0)` -> D2Common `#10554(0,1,0)`;
   - shutdown: D2Common `#10553` -> D2Lang `#10001` -> D2Common `#10983`;
   - #10983 is identified by its binary strings as `D2Common\Logging\ProfCore.cpp` profiling dump code, not an additional destructor.

4. **True D2Game/transport startup split**
   - D2Game `#10046` runs first;
   - MCP and CharServ are established next;
   - only after transport initialization does retail install D2Game `#10023`, then `#10002`, then `#10039`.
   - v0.4 now mirrors this split instead of bootstrapping all D2Game state before transport.

5. **Worker-count source**
   - Fog `#10020` returns a process/system descriptor;
   - D2Server reads descriptor `+0x28`, exactly `SYSTEM_INFO.dwNumberOfProcessors`;
   - v0.4 uses that source with a safe host fallback only for incomplete portable environments.

6. **Exact core shutdown order**
   After workers retire:
   - retail UI/list cleanup `0x100055B0` (headless shell does not own this);
   - D2Game `#10040`;
   - D2Net `#10004`;
   - online-service path: D2Net `#10018(NULL)`, destroy MCP, destroy CharServ clients;
   - D2Game `#10050`;
   - D2Common `#10553`;
   - D2Lang `#10001`;
   - D2Common `#10983` profiling dump;
   - retail releases two application-shell objects at `0x10001830` (not created by this headless source).

7. **D2Net #10018 callback ABI**
   - Fog invokes the callback value in ECX;
   - it is therefore modeled as an x86 register/fastcall callback rather than an ordinary stdcall stack parameter.

8. **MULTICLIENT zero behavior**
   - the parser accepts stored values 0..1024;
   - the retail getter at `0x10007580` maps stored zero to 1;
   - explicit stored zero therefore reaches D2Net #10026 as 1, matching the original.

## Portable regression status

Final clean suite: **10/10 passing**.

1. `player_tokens`
2. `layouts`
3. `bridges`
4. `realm_config`
5. `worker_loop`
6. `fog_transport`
7. `d2net_startup`
8. `teardown_order`
9. `engine_globals`
10. `startup_phases`

`startup_phases` specifically locks the retail order `D2Game #10046 -> transport boundary -> D2Game #10023`. `teardown_order` locks the core reverse sequence and matched engine-global shutdown.

Release verification was repeated in three configurations: the normal portable build, a strict `-Wall -Wextra -Wpedantic` build with zero warnings, and AddressSanitizer + UndefinedBehaviorSanitizer. **All three configurations pass 10/10 tests.** The sanitizer run found an earlier fixed-length PlayerToken source read; the final code now follows the retail Storm #501 bounded 16-byte string-copy semantics.

## Final v0.4 boundary

The service core is now internally coherent enough to justify a real Win32 test. Work intentionally left outside this release is application-shell parity rather than a known D2Game/D2Net lifecycle hole:

- Display.cpp/status-window implementation and its UI list;
- Bnclient auxiliary shell behavior;
- D2Hell/Archive status/enumeration glue;
- retail performance/status presentation around the headless service loop;
- deeper MCP 0x03/0x12 status semantics;
- live repeated lifecycle/reconnect proof.

The next milestone should be driven by the first Win32 run and its logs rather than adding more speculative shell code before runtime evidence exists.
