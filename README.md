# D2Server1.00 Reconstructed

Source-equivalent reconstruction of the accidentally shipped Diablo II 1.00 `D2Server.dll`, checked directly against the exact matching May 26, 2000 retail DLL family supplied for this project.


Target retail D2Server:

- 184,320 bytes
- SHA-256 `ccaaeb7800376991482e74f4e0e2c01f16088a197d45d38f39eb16ee50c2513b`
- linked 2000-05-26 19:15:40 UTC

## v0.4 retail-backed runtime order

```text
Fog #10139(1)                         -> Fog global server-mode flag
Fog #10019("D2SDebug")               -> native ErrorManager initialization
realms.ini / ACCEPT realm selection
        |
D2Lang   #10000(0)
D2Common #10554(0,1,0)               -> global data-table initialization
        |
D2Net #10003(0,0)                    -> game TCP layer / port 4000
D2Net #10026(MULTICLIENT getter; stored 0 becomes 1)
D2Net #10023(1)
        |
D2Game #10046                        -> engine runtime phase 1
        |
Fog MCP client      :6112            -> configure -> start -> control 02 -> framed 01
D2Net #10018(hack-report callback)
Fog CharServ pool   :6113            -> up to 3 x 0x2C slots -> control 05
        |
D2Game #10023(12 callbacks)          -> engine runtime phase 2
D2Game #10002(GameDataHash)
D2Game #10039
        |
N D2Game workers, N = SYSTEM_INFO.dwNumberOfProcessors
  D2Game #10041 task queue
  D2Game #10043 next due task
  D2Net  #10022 wait/pump
  D2Game #10003 network processing
  D2Game #10045 due game task
        |
headless service tick
  poll CharServ
  drain MCP
  30/60-second service health behavior
  MCP/CharServ health + reconnect
  Sleep(1 ms)
        |
MCP opcode 13 -> stop request
        |
join all workers
D2Game   #10040
D2Net    #10004
D2Net    #10018(NULL) + destroy MCP/CharServ Fog clients
D2Game   #10050
D2Common #10553
D2Lang   #10001
D2Common #10983                      -> ProfCore profiling dump
```

The original WinMain performs GUI/status/application-shell work around this service core. v0.4 intentionally substitutes a headless service loop for that shell rather than inventing Display/Bnclient/D2Hell behavior.

## Major v0.4 additions over v0.3

- complete 25-DWORD / 0x64-byte x86 QueryInterface block;
- exact 1.00 D2Net ordinal map, startup sequence and shutdown control path;
- restored Fog #10139 global-mode call and Fog #10019 `D2SDebug` ErrorManager initialization;
- restored D2Lang #10000 / D2Common #10554 startup and matching #10553 / D2Lang #10001 teardown;
- identified D2Common #10983 correctly as the final ProfCore profiling dump, not a destructor;
- split D2Game bootstrap around the realm transports exactly as retail does: #10046 first, then MCP/CharServ, then #10023/#10002/#10039;
- exact D2Game worker/task cadence and processor-count worker source;
- real Fog MCP connection on TCP 6112;
- real Fog Character Server pool on TCP 6113;
- three 0x2C CharServ slots, round-robin send/failover, health checks and reconnect path;
- CharServ opcode 3 -> exact five-argument D2Game #10007 character-delivery bridge;
- `realms.ini` realm selection, ACCEPT rules and MULTICLIENT getter behavior;
- central blocking service loop and MCP remote-stop integration;
- retail-backed shutdown order through D2Game/D2Net/Fog/D2Common/D2Lang;
- fallback diagnostics in `D2Server100_v0_4.log` in addition to Fog's native ErrorManager.

v0.3 engine work remains integrated: SGAMEDATA hash/container and Storm allocator hooks, CREATEGAME, all 12 early D2Game callbacks, PlayerToken, MCP dispatcher, save checksum, ladder state, Guild chunk persistence, and the D2Game bootstrap ordinals.

## Portable verification

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Final v0.4 source release result:

```text
player_tokens    PASS
layouts          PASS
bridges          PASS
realm_config     PASS
worker_loop      PASS
fog_transport    PASS
d2net_startup    PASS
teardown_order   PASS
engine_globals   PASS
startup_phases   PASS

10/10 tests passed
```

The final tree was also verified with a strict `-Wall -Wextra -Wpedantic` build (zero compiler warnings) and with AddressSanitizer + UndefinedBehaviorSanitizer; all 10 tests pass under both. The sanitizer pass exposed and helped correct the PlayerToken fixed-field copy, which is now modeled on the retail Storm #501 bounded string-copy call. See `research/PLAYER_TOKEN_STORM501_COPY.md`.

## Build the Win32 DLL

Build **Win32/x86**, never x64. With Visual Studio 18 2026:

```bat
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Release --target D2Server
ctest --test-dir build -C Release --output-on-failure
```

Expected DLL:

```text
build\Release\D2Server.dll
```

Verify exports:

```bat
dumpbin /exports build\Release\D2Server.dll
```

Expected ordinals:

```text
10001  NONAME
10002  NONAME
10003  QueryInterface
```


See `STATUS_v0_4_FINAL.md` and the `research/` directory for the evidence trail.
