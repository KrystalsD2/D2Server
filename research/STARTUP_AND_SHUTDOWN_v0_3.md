# Engine bootstrap recovered for v0.3

The original 1.00 WinMain path performs, among other shell work, this D2Game sequence:

1. D2Game #10046 - runtime initialization.
2. D2Game #10023(callbackTable) - store 12-entry server callbacks.
3. D2Game #10002(gameDataHash, nonNull) - install game-data structures.
4. D2Game #10039() - post-game-data synchronization initialization.

v0.3 now reproduces that engine-facing sequence.

The original shutdown path reaches at least:

- D2Game #10040()
- D2Game #10050()

v0.3 exposes these through ShutdownEngine100(). Full WinMain-owned connection/thread/resource teardown is still outside this reconstruction.
