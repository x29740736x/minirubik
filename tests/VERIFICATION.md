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

The host harness uses `-std=c99 -O2 -Wall -Wextra`. Executables and regenerable
full-domain distance binaries are excluded. Follow h3/README.md to rerun.