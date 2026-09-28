# Exact 1.00 D2Net import/xref map from retail D2Server

Retail D2Server statically imports six D2Net ordinals:

- D2Net #10018 -> D2Server thunk `0x1000BFC4`
- D2Net #10022 -> `0x1000BFCA`
- D2Net #10004 -> `0x1000BFD0`
- D2Net #10003 -> `0x1000BFE2`
- D2Net #10026 -> `0x1000BFDC`
- D2Net #10023 -> `0x1000BFD6`

Confirmed retail D2Server xrefs:

- #10018: `0x10001949`, `0x10001A76`, `0x10001AEB`
- #10022: `0x10009D8F`, `0x10009E1D`
- #10004: `0x1000A13C`
- #10003: `0x10009F5C` initialization call
- #10026: `0x10009F67`
- #10023: `0x10009F6E`

## Startup stack interpretation

At `0x10009F58`, retail WinMain pushes two zero DWORDs and calls thunk `0x1000BFE2`. The PE import/IAT map proves that thunk is D2Net #10003, whose implementation returns with `ret 8`. Retail then calls the MULTICLIENT getter, pushes its result to thunk `0x1000BFDC` = #10026, and finally pushes literal 1 to thunk `0x1000BFD6` = #10023.

Therefore the actual logical sequence is:

```text
D2Net #10003(0, 0)
D2Net #10026(multiclient_getter())
D2Net #10023(1)
```

The return value from #10003 is not tested by retail WinMain. D2Net #10003 itself hard-wires protocol/type 2 and TCP port 4000 when it calls into Fog.

## Shutdown xref

Retail core shutdown reaches #10004 immediately after D2Game #10040, before clearing #10018 and destroying Fog MCP/CharServ objects. See `WINMAIN_TEARDOWN_DISASM.txt`.
