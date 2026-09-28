# Integrated v0.4 runtime flow - final

Verified against the exact retail 1.00 D2Server/D2Net/D2Game/Fog/D2Common/D2Lang family.

```text
QueryInterface.start
  -> Fog #10139(1)
  -> Fog #10019("D2SDebug")
  -> parse/select realms.ini block by local-IP ACCEPT rule
  -> D2Lang #10000(0)
  -> D2Common #10554(0,1,0)
  -> D2Net #10003(0,0)
  -> D2Net #10026(MULTICLIENT getter)
  -> D2Net #10023(1)
  -> D2Game #10046
  -> Fog MCP :6112 + configure callback + start + control 02 + framed 01
  -> D2Net #10018(hack-report callback)
  -> Fog CharServ :6113 + up to three slots + control 05
  -> D2Game #10023(12 callbacks)
  -> D2Game #10002(SGAMEDATA hash)
  -> D2Game #10039
  -> create one D2Game worker per SYSTEM_INFO.dwNumberOfProcessors
  -> headless 1 ms service loop
       poll CharServ
       drain MCP
       periodic health/reconnect behavior
  -> MCP opcode 13 requests stop
  -> wait/join all game workers
  -> D2Game #10040
  -> D2Net #10004
  -> D2Net #10018(NULL)
  -> destroy MCP Fog client
  -> destroy CharServ Fog clients
  -> D2Game #10050
  -> D2Common #10553
  -> D2Lang #10001
  -> D2Common #10983 ProfCore dump
```

## MULTICLIENT zero behavior

The parser accepts signed decimal values 0..0x400 into storage. The retail getter at D2Server VA `0x10007580` reads the stored value and returns 1 when it is zero. Therefore D2Net #10026 receives 1 for both an unset value and an explicit stored zero.

## Worker count

Retail WinMain calls Fog #10020, receives Fog's process/system descriptor and reads `descriptor+0x28`. The embedded SYSTEM_INFO starts at +0x14, making +0x28 `SYSTEM_INFO.dwNumberOfProcessors`. v0.4 uses that exact source where available and a safe host processor-count fallback if the portable environment cannot expose it.

## MCP service-thread equivalence

Retail creates a dedicated online-service thread at `0x10009B80`. That thread drains Fog MCP receive data, invokes the main MCP dispatcher, performs periodic service checks, and sleeps for 1 ms. v0.4 intentionally folds those responsibilities into its headless central service tick instead of recreating the Windows GUI/thread presentation literally. The transport semantics remain separate from the D2Game workers.

## Application-shell boundary

Retail also initializes QueryPerformanceCounter/status state and updates its Windows application shell after the game workers are started. It performs a GUI/list cleanup at `0x100055B0` immediately before D2Game #10040 and releases two shell-owned objects at `0x10001830` after the final ProfCore dump. Those shell objects are not created by v0.4 and are intentionally not destroyed by it. The matched D2Lang/D2Common global lifecycle is part of the reconstructed service core and is implemented.
