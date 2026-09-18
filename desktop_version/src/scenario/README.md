# Reference scenario harness

This directory adds a way to run a *scenario* (settings, start state, and the buttons held on each frame) on the unmodified game and record the per-frame game state to a *trace*. A reimplementation can then check itself against the trace frame by frame.

The scenario and trace formats are specified in `scenarios/FORMAT.md` of the [fe6](https://github.com/tzann/fe6) repository, which also holds the scenarios, the recorded traces and the Rust checker.

```
VVVVVV -scenario <file.toml> [-trace <out.jsonl>] [-scenario-hidden] [-scenario-realtime]
VVVVVV -scenario-check-rand
```

Without `-scenario`, the game behaves exactly like 2.4.4.

## Changes to the game's own files

All changes to game files are insertions, each marked `SCENARIO`:

- **`main.cpp`**
  - include `scenario/Scenario.h`
  - `scenario_step()`: one call to the real `deltaloop()`, driven by a virtual clock
  - `scenario_loop_state()`: reports the loop bookkeeping
  - `SCENARIO_parse_args()` after `vlog_init()`
  - `SCENARIO_setup()` before the window is shown
  - `SCENARIO_run()` instead of the main loop, when a scenario is active
- **`Xoshiro.c`**: `xoshiro_get_state()`, a read-only accessor
- **`CMakeLists.txt`**: builds the files in this directory. They are compiled as C++17; the game's own files keep their flags.

Check with `git diff 2.4.4 -- desktop_version/src desktop_version/CMakeLists.txt`.

## Files

| File | Purpose |
|---|---|
| `Scenario.cpp` | command line, settings, start procedures, input injection, main run loop |
| `ScenarioFile.cpp` | scenario validation, input log |
| `ScenarioToml.cpp` | the TOML subset parser |
| `ScenarioProbes.cpp` | the recorded variables ("probes") and `[start.set]` writes |
| `ScenarioTrace.cpp` | delta-encoded JSON Lines writer |
| `ScenarioRand.c` | Windows: a `rand()`/`srand()` bit-identical to the Microsoft CRT, with observable state |
| `ScenarioRandCheck.c` | `-scenario-check-rand` |

## Building

Build it like upstream 2.4.4. It needs SDL2 **2.26** or newer: 2.4.4 calls `SDL_GetWindowSizeInPixels`, so an SDL2.dll from 2.24 does not work.

- **MinGW-w64 (tested; the recommended reference binary).** Cross-compiled from Linux: see `tools/build_reference.sh` in fe6.
- **MSVC (not tested here).** Use Visual Studio 2019 or newer, with SDL2-devel-2.26.x-VC:
  ```
  cmake -S desktop_version -B build -A x64 -DSDL2_INCLUDE_DIRS=C:/SDL2-2.26.5/include -DSDL2_LIBRARIES="C:/SDL2-2.26.5/lib/x64/SDL2.lib;C:/SDL2-2.26.5/lib/x64/SDL2main.lib"
  cmake --build build --config Release
  ```
  Run `VVVVVV.exe -scenario-check-rand` afterwards. With the default `/MD` runtime, MSVC may bind `rand()` to the CRT import instead of the harness' copy. The harness detects this and leaves the `rng.crt.*` probes out of the trace; nothing else changes.

To run scenarios, put `data.zip`, `lang/` and `fonts/` next to the executable, as in a release. The `lang/` and `fonts/` directories come from `desktop_version/`.
