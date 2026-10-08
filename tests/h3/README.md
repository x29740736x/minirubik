# Exhaustive verification of the supplied vin_solver.c

This host-only harness includes `vin_solver.c` unchanged (its CLI main is renamed
by a macro). It calls the actual `ida_search`, uses the same increasing thresholds
as `ida_solve`, and retains each returned path for validation.

`oracle.c` includes the original `solver_baseline.c` in a separate translation
unit. It constructs an exact BFS distance table using the baseline's ranking,
unranking and quarter-turn functions. Returned paths are replayed using baseline
moves, not the modified solver's move implementation. The oracle is never linked
into the Ripes ELF.

## Build and run in VS Code PowerShell

```powershell
gcc -std=c99 -O2 -Wall -Wextra verify_all.c oracle.c -o verify_all.exe
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }

# First verify 100 states. This is NOT an exhaustive H3 result.
.\verify_all.exe 0 100 smoke_result.json

# Then test all 3,674,160 states. This may take a long time for the
# unchanged cubie-array search; it is slower than coordinate-table search.
.\verify_all.exe 2>&1 | Tee-Object -FilePath h3_full.log
Get-Content .\h3_result.json
```

Each invocation first checks the oracle's state coverage and depth histogram,
the supplied distance tables' population and solved entries, and heuristic
admissibility plus rank/unrank agreement over the entire domain. Then H3 tests
only the requested rank interval: exact optimal length and baseline path replay.
The reported H3 wall-clock time excludes oracle construction and the preliminary
checks. The command's total wall-clock duration includes them.

All-state PASS requires `status` to be `PASS`, `verified_this_run` to be 3674160,
and `complete_entire_domain` to be true. A partial interval never reports full
completion, even if every state in that interval passes.

The JSON progress file is updated every 1000 verified states. Ctrl+C stops the
process. If resuming, preserve the original JSON/log and run the remaining range
with a new output filename, e.g.:

```powershell
# Example only: replace 100000 with the saved next_rank.
.\verify_all.exe 100000 3574160 h3_remaining.json
```

Separate intervals constitute exhaustive coverage only when their recorded,
successful ranges cover [0,3674160) with no gaps and use identical source files.
Add their H3 times when reporting a segmented run; do not claim that the last
segment alone validated every state. Build/source SHA-256 hashes should be
recorded before testing and the sources must stay unchanged during the run.

## Scope

- Checks the supplied **cubie-array C search**, not a substituted faster search.
- Does not prove the assembly coordinate-search H3 gate or its instruction cost.
- Does not execute Ripes or measure retired instructions.
- Host heap allocation and ordinary multiplication/division are allowed here.
- H4 is not applicable: the supplied distance tables are unpacked bytes.
- H2 here checks the retained distance tables, not every target transition table.

The older MinGW compiler in the assistant's execution environment could not
start (Windows DLL relocation error). Until a native compiler run is completed,
do not describe this harness or exhaustive H3 as tested or passed.
