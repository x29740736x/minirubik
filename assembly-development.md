# Assembly Development — Design and Execution Flow

This supplement explains the implementation choices behind the main report. The final target search uses coordinate tables; it is not a line-by-line translation of the earlier cubie-array C search.

## 1. Input and State

The parser reads a 14-character string, subtracts ASCII '1', and stores seven permutation bytes followed by seven orientation bytes. It checks the length, ranges, duplicate corners using a bit mask, and orientation sum modulo three. Invalid input exits with status 1.

## 2. Cubie Moves and Coordinate Ranks

R, B, and D share a source/twist loop. Results are written to next_state before copying back, so writes cannot overwrite corners still being read. R2 and R′ use two and three quarter-turns. Move decoding uses subtraction instead of division by three.

rank_p counts smaller corners to the right; rank_o encodes the first six orientations in base three using shifts and addition. These routines rank the input and verify replay. They are not called to reconstruct every search node.

## 3. Table-Based Search

The host tool builds two distance tables and two nine-move transition tables. The assembly searches the full problem on the target while using these smaller tables to update coordinates and obtain max(h_P,h_O).

Each transition row has nine two-byte entries. The target calculates table_base + 18*rank + 2*move without multiplication. The initial cubie-array approach was easier to inspect, but coordinate lookups avoid repeated array moves and ranking during search.

## 4. Explicit Search Frames

The iterative search stores P/O ranks, the next move, and the previous face for each depth. It advances the parent's next move before descending, so backtracking resumes at the next candidate.

| Register | Search role |
|---|---|
| s2, s5 | Current P/O ranks |
| s3, s4 | Depth and threshold |
| s0, s1 | P/O distance-table bases |
| s10, s11 | P/O transition-table bases |
| s8, s9 | Coordinate-frame and next-move pointers |

```text
Parse and validate
→ Back up input and rank P/O
→ Start threshold at the heuristic
→ Check g+h and solved coordinates
→ Try a move, skipping the previous face
→ Look up child coordinates and descend
→ Backtrack after pruning or exhausting moves
→ Increase threshold if necessary
→ Replay, verify, and print the solution
```

Keeping table bases and frame pointers in registers reduced repeated loads, helper calls, and address calculations. The required-vector count fell from 24,902,514 to 16,947,844; the difficult case fell from 68,086,180 to 46,333,284. These are non-LED RV32_ISS measurements. No earlier linked code-size measurement was retained.

## 5. Calls and Replay

Functions that call other functions save ra and restore any saved registers they use. This bounded call stack is separate from recursive search, which has been removed.

After finding a solution, s8 and s9 are reused as replay index and solution length. The original cubie state is restored and each solution_path move is applied. Both final coordinate ranks must be zero before the program prints a solution.

## 6. LED Integration

The LED work was developed separately: first a pixel, then a cube net, individual stickers, R/B/D cycles, move decoding, and P/O-to-sticker conversion. Four-turn and inverse tests checked restoration; single R/B/D states checked consistency with cubie decoding.

ida_found converts the original state into sticker colors and draws it. After each cubie replay move, replay_loop reloads the move from solution_path, updates the sticker array, redraws the net, and delays briefly. Reloading is necessary because apply_move changes argument registers.

The first GUI attempts displayed a stale test program even though the editor contained new source. After reopening Ripes and loading the source, the disassembly matched the input parser and the one-move solve displayed correctly. The exact cause of the stale assembly state was not established.

## 7. AI-Assisted Development

AI generated and revised parts of the C, assembly, LED integration, report, and test tools. I applied suggestions in steps and checked their behavior through the recorded tests. The LED integration was revised to remove unsupported or unverified editor build assumptions and to reload the actual replay move. Code and measurements remain linked from the main report; this supplement describes the process rather than asserting that all work was independently authored.
