# Host verification evidence

The cubie-array `vin_solver.c` full-domain run was reported in the development
terminal as:

```text
PASS: 3674160 states; optimal lengths and baseline replay; 2748.299 wall seconds
```

This line is transcribed from the user's terminal output. Subsequent reruns
overwrote the original full-run JSON and log; the current incomplete files are
excluded from this commit. This is not a newly reconstructed machine result.
The source snapshots and commands are retained in `h3/` for reproduction.

`h3-two-checks/` provides a second harness using the supplied solver for BFS and
replay. Its committed smoke results cover only their explicitly recorded ranges.

`po-coordinate/po_result.json` is the complete coordinate-search run: 3,674,160
optimal lengths and 3,674,160 replays, 477.411 seconds. That run validates the
coordinate variant, not the unchanged cubie-array search. Its BFS and replay
reuse vin_solver.c and are not independent implementations.

All host harnesses use `-std=c99 -O2 -Wall -Wextra`. Executables and regenerable
full-domain distance binaries are excluded. Follow each harness README to rerun.