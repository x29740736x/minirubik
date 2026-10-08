# P/O coordinate IDA* host prototype

The original supplied vin_solver.c is preserved unchanged. po_search.h reuses
its parser, coordinate ranking, cubie moves and heuristic-table builders.
solver_po.c runs a single input. verify_po.c checks a range against the saved
host BFS table and replays EVERY returned solution with the cubie implementation.

Build: .\compile.ps1
Solve: .\solver_po.exe 21345671111111
Smoke: .\verify_po.exe 0 10000 po_smoke_10000.json
Full: .\verify_po.exe 2>&1 | Tee-Object -FilePath po_full.log
Result: Get-Content .\po_result.json
Rebuild exact distances after changing source: .\build_table.exe

How the search changes:
1. rank_p and rank_o run only at the root, not at each child.
2. Each frame stores P rank, O rank, next move and previous face.
3. A child uses p_transition[9*P+move] and o_transition[9*O+move].
4. The heuristic is still max(pd[P],od[O]); thresholds and move order are unchanged.
5. After finding a solution, apply_move on cubie arrays verifies the actual path.

The separate nine-move tables occupy 90,720 bytes (P) and 13,122 bytes (O).
P/O distances occupy 5,040 and 729 bytes, giving 109,611 bytes for these four
buffers. This is NOT the total program/process memory. There are no combined
state transition entries or full-state distances in the search algorithm.
The 3,674,160-byte exact_distances.bin is host verification only, never target data.

This is a host C prototype, not a finished audited RV32I reference build.
Host startup constructs the transition and heuristic tables at runtime. A target
version may instead include host-generated read-only tables. Unsupported target
instructions/helper calls and final linked section sizes still need checking.
The complete original source is included for reuse, including unused old entry
points; this package is not a minimal bare-metal target program.

Verified with GCC -std=c99 -O2 -Wall -Wextra:
- Solved input 12345671111111: 0 moves, replay PASS.
- Short input 25314672313211: 1 move R', replay PASS.
- Required input 21345671111111: 11 moves, replay PASS.
- First 10,000 ranks: all optimal lengths and all replays PASS.
Single-run timings for the same [0,10000) range with full replay and -O2:
original cubie-array verifier 8.952 s; coordinate verifier 1.530 s (~5.85x).
These are host wall times excluding table setup; not retired instruction counts
and not exhaustive results or a guaranteed speedup on every input.
No complete all-state run of this new coordinate implementation has been done.

BFS and replay use the same supplied cubie move definitions. Preserve the earlier
independent baseline test as separate evidence. Old cubie-array H3 PASS does not
automatically validate this changed coordinate search; run verify_po for all ranks.
