# Host H3: exactly two per-state checks

This package uses only the unchanged supplied vin_solver.c. No professor source.

1. Recompile: .\compile.ps1
2. Rebuild the BFS table (already included): .\build_table.exe
3. Smoke test: .\verify_again.exe 0 100 smoke_result.json
4. Full run: .\verify_again.exe 2>&1 | Tee-Object -FilePath two_checks_full.log
5. Read result: Get-Content .\two_checks_result.json

Every requested state is searched by the original iterative IDA* with increasing
thresholds. Check 1 compares the returned length to the saved full BFS distance.
Check 2 replays every returned path with the supplied apply_move and compares
both cubie arrays to the solved state. There is no sampled/no-replay mode.
No extra exhaustive H1 or self-test sweep is performed. The saved table's size,
value range, solved entry and histogram are checked when it is loaded.

exact_distances.bin is a raw 3,674,160-byte host-only table indexed by rank.
It was generated from the same vin_solver.c source; this is not an independent
check of the cube move definitions. Never link this full-state table into RV32I.
Rebuild both executables and the table after changing the solver source.

Compiled successfully with GCC and explicit -std=c99 -O2 -Wall -Wextra.
compile.ps1 records flags, source hashes and executable hashes in build_info.json.
Assistant ran the table builder (0.103 s), the first 100 states with full replay
(PASS, 0.137 s), and required state 21345671111111 at rank 524880 (PASS).
This NEW harness has not yet completed a full all-state search. Earlier full
verification results belong to earlier harnesses and must remain separate.

Full verification time excludes loading the table and building P/O heuristics.
The result's all_state_lengths_verified and all_state_paths_replayed must both
be true, with status PASS, for a complete run. Partial-range PASS is not full PASS.
