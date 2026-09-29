# D2Server100-Reconstructed v0.4.1 — Game.exe host-contract correction

This checkpoint corrects the original full-service host ABI against the supplied retail Diablo II 1.00 `Game.exe` and `D2Server.dll`.

## Corrections

- `QueryInterface()` slot 0 receives the parsed **0x376-byte Game.exe startup/config object in ECX**, not a `const char*` command line.
- Retail D2Server consumes only three meaningful host fields from that object:
  - `NETWORK.BATTLENETIP` at `+0x46`: stores `firstByte != '0'`; no later service-core read is proven.
  - `NETWORK.MCPIP` at `+0x5E`: literal ASCII `"0"` disables the Blizzard online-service block; otherwise it seeds the working MCP endpoint.
  - `DEBUG.LOG` at `+0x1EE`: enables D2Common debug logging through ordinal `#10980` with the enable value in ECX.
- The online-service gate is checked around the Blizzard **MCP :6112 + Character Server :6113** transport block. D2Game bootstrap/workers continue on the common path when it is disabled.
- A selected `realms.ini` `mcpip=` overrides the host MCPIP default when present. It does not re-enable an MCPIP=`0` host.
- `adminpass` and `publicpass` remain parsed/stored for binary parity, but no service-core read and no use in the observed MCP registration exchange is proven.
- `bnetip` remains a required selected-realm field, but no GS->BNCS socket consumer is proven in retail D2Server.

## Port/role boundary

Do not equate the original Blizzard ports/protocols with PvPGN's later layout.

Original retail D2Server full-service path:
- Blizzard MCP / realm-control: TCP **6112**
- Blizzard Character Server: TCP **6113**

RetroD2/PvPGN production:
- D2GS -> D2CS: TCP **6113**
- D2GS -> D2DBS: TCP **6114**

The responsibilities are historically related, but the protocols and port assignments are not interchangeable.

## Validation-harness correction

v0.4's CMake allowed `NDEBUG` to remove assert expressions in Release builds even though several tests intentionally perform setup inside those expressions. v0.4.1 explicitly undefines `NDEBUG` for test executables, matching the later 1.06b branch and making Release validation meaningful.
