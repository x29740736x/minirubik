# Host verification evidence

The cubie-array vin_solver.c IDA* search passed the complete H3 rerun:

```text
PASS: 3674160 states; optimal lengths and baseline replay; 2686.302 wall seconds
```

For every state, the harness checked the returned solution length against an
exact full-state BFS distance and replayed the moves using the original baseline
implementation. All 3,674,160 states passed both checks.

- Full result: [h3_complete_result.json](h3/h3_complete_result.json)
- Full execution log: [h3_complete_run.log](h3/h3_complete_run.log)
- Source snapshots and commands: [h3/README.md](h3/README.md)

The result records status PASS, start_rank 0, next_rank 3674160,
verified_this_run 3674160, complete_entire_domain true, and
h3_wall_seconds 2686.301595. This time excludes BFS construction and setup.
The new files replace the previously reported run whose result files were
subsequently overwritten.

The host harness uses -std=c99 -O2 -Wall -Wextra. Host executables are excluded
from Git. The full-state oracle is used only on the host, not in the RV32I ELF.