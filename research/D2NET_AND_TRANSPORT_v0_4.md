# D2Net / Fog transport findings used by v0.4

Verified against the matching May-26-2000 retail binaries supplied for this reconstruction.

## D2Net imports used by retail D2Server

- #10003: two stack arguments, `ret 8`; server initialization; D2Net hard-wires the game service path internally.
- #10004: no arguments; shutdown/control path. Its D2Net body builds the five-byte internal control frame `D2 00 05 00 01`.
- #10018: one stack argument; installs/clears the hack-report callback. Fog later invokes the callback value through ECX, making the callback itself x86 fastcall/register ABI.
- #10022: one stack argument; wait/network pump used by the game worker.
- #10023: one stack argument; startup value is 1.
- #10026: one stack argument; receives the MULTICLIENT getter result.

Exact retail startup order at D2Server `0x10009F58..0x10009F73`:

```text
#10003(0,0)
#10026(GetMultiClient())
#10023(1)
```

There is no cross-call stack trick here. The PE import table maps the call thunks directly: `0x1000BFE2` is #10003, `0x1000BFDC` is #10026, and `0x1000BFD6` is #10023. Retail WinMain pushes two zeroes for #10003, then separately passes the MULTICLIENT getter result to #10026 and literal 1 to #10023.

## MULTICLIENT getter

Retail D2Server `0x10007580`:

```text
mov eax,[stored_multiclient]
test eax,eax
jne return
mov eax,1
return:
ret
```

So stored 0 maps to 1 at the D2Net call boundary.

## D2Game worker ordinals

- #10041: create task queue.
- #10043: fastcall `(queue, &dueTask)`, returns wait milliseconds.
- #10003: process pending network messages.
- #10045: fastcall `(queue, dueTask)`, process due game task.

D2Server clamps negative waits to zero. When due tasks are continuously present it still pumps D2Net on the first task and every eighth task.

Worker count comes from Fog #10020 process/system descriptor +0x28 = `SYSTEM_INFO.dwNumberOfProcessors`.

## Fog startup and client transport

Before realm loading, retail D2Server calls Fog #10139 with ECX=1 (a one-instruction global-mode setter) and Fog #10019 with the literal `D2SDebug` to initialize Fog's ErrorManager. v0.4 restores both calls.

The same Fog client family then carries both realm services:

- MCP: TCP 6112, control hello `02`; then framed one-byte `01`.
- Character Server: TCP 6113, control hello `05`.

Relevant Fog exports:

- #10047 create client object (`ret 0x0C`)
- #10048 destroy client (`ret 4`)
- #10049 framed send (`ret 0x0C`)
- #10050 control send (`ret 0x0C`)
- #10051 receive/poll (`ret 0x0C`)
- #10052 initial connection worker (`ret 4`)
- #10053 reconnect worker (`ret 4`)
- #10056 configure callback/context (`ret 0x0C`)
- #10058 active transport accessor (`ret 4`)

MCP startup configures the Fog callback, starts the connection, sends control `02`, sends framed `01`, then installs the D2Net #10018 hack callback. Retail performs MCP and initial CharServ setup after D2Game #10046 but before D2Game #10023/#10002/#10039; v0.4 now preserves that split.

## Character Server slots

Retail slot size is 0x2C:

```text
+00 connection pointer
+04 char address[36]
+28 reconnect-thread handle
```

Maximum three slots. Sends fail over round-robin beginning with the current primary. The reconstructed receive dispatcher meaningfully handles opcodes 3, 5 and 8. Opcode 3 forwards fields +05/+09/+0B/+0D and bytes +11 to the exact five-stack-argument D2Game #10007 bridge.

## Retail core shutdown order

After worker exit:

```text
D2Game #10040
D2Net  #10004
D2Net  #10018(NULL)
Fog #10048(MCP)
Fog #10048(CharServ slots)
D2Game   #10050
D2Common #10553
D2Lang   #10001
D2Common #10983  (ProfCore final dump)
```

This order is asserted by `tests/teardown_tests.cpp`; the matched D2Lang/D2Common lifecycle is separately exercised by `tests/engine_globals_tests.cpp`.
