# Assignment1 Note

# Stage 1 — Characterize the Baseline

The first stage is to analyze the original `minirubik` implementation and identify where its computational cost comes from, especially in terms of **memory usage, instruction count, and algorithm design**.

## 1.1 Original Algorithm

The original implementation uses Breadth-First Search (BFS), starting from the solved state, to explore all 3,674,160 valid states in the fixed-reference-corner representation of the 2×2×2 Rubik’s Cube.

For each visited state, it considers nine moves. When a new state is first discovered, the program adds its rank to the BFS queue and stores the inverse move in `toward_solved`. Each table entry therefore records the next move toward the solved state, rather than the state’s distance.

Since BFS explores states in increasing depth, following the recorded moves produces a shortest solution. The program tracks BFS levels separately to determine the maximum distance, which is 11 moves.

The main cost is constructing the complete table before solving an input, requiring both a large move table and a large BFS queue.




## 1.2 Cube Representation

The original program represents a 2×2×2 Rubik's Cube using two parts: **Permutation (P)** and **Orientation (O)**.

In the original C implementation, a cube state is represented by:

```c
typedef struct {
    uint8_t p[CUBIES];
    uint8_t o[CUBIES];
} state_t;
```

### Permutation (P)

A 2×2×2 Rubik's Cube contains 8 corner cubies.

In this implementation, the front-upper-left corner is fixed as a reference and omitted from the seven-corner representation.

The `p[]` array records **which corner cubie is located at each position**. Therefore, `p[]` describes the permutation of the 7 movable corner cubies.

Since there are 7 movable corner cubies, the total number of possible permutations is

$$
7! = 5040
$$

Only one of these permutations corresponds to the solved arrangement.

### Orientation (O)

The `o[]` array records the **orientation of each corner cubie**.

Each corner cubie has three possible orientations, represented by `0`, `1`, and `2`.

However, the orientations of all 7 movable corners are not independent. Due to the corner orientation constraint of the cube, once the orientations of 6 corners are known, the orientation of the final corner is determined.

Therefore, the number of possible orientation states is

$$
3^6 = 729
$$

### Complete State Representation

A complete cube state is determined by both its permutation and orientation.

Therefore, the total number of possible states is

$$
7! \times 3^6
= 5040 \times 729
= 3,674,160
$$

In summary:

- **P (Permutation):** describes which corner cubie is located at each position.
- **O (Orientation):** describes the orientation of each corner cubie.
- Together, **P and O uniquely represent a cube state**.



## 1.3 State-Space Invariants

The state representation relies on several invariants that remain valid after every legal move.

### Fixed Reference Corner

A 2×2×2 Rubik's Cube has 8 corner cubies. In this implementation, the front-upper-left corner is fixed as a reference corner.

The program only uses the `R`, `B`, and `D` faces to represent cube moves. These moves do not move the reference corner.

Therefore, only the remaining 7 corner cubies need to be represented and permuted.

This reduces the number of possible corner permutations from

$$
8!
$$

to

$$
7! = 5040
$$

### Corner Orientation Constraint

Each movable corner has three possible orientations:

$$
o_i \in \{0,1,2\}
$$

However, the orientations of the 7 movable corners are not independent.

For every legal cube state, the sum of the corner orientations must satisfy

$$
\sum_{i=1}^{7} o_i \equiv 0 \pmod{3}
$$

Therefore, once the orientations of 6 corners are known, the orientation of the final corner is determined automatically.

As a result, only 6 corner orientations need to be stored independently, giving

$$
3^6 = 729
$$

possible orientation states.

### Valid State Space

Because the fixed reference corner and the corner orientation constraint are preserved by every legal move, the program only needs to consider

$$
7! \times 3^6 = 3,674,160
$$

valid states instead of all possible arbitrary arrangements of the 8 corners.

### State Graph and Diameter

The fixed-corner states form the group $G = \langle R, B, D \rangle$ of order $7! \times 3^6 = 3,674,160$. Its Cayley graph has one vertex per state and edges for the nine moves R, R2, R′, B, B2, B′, D, D2, and D′. Each edge costs one move in the half-turn metric (HTM).

The host BFS reached the complete state space and found a maximum distance of 11, with 2,644 states at that distance. Full coverage establishes that none is farther away, and the nonempty distance-11 level establishes that 11 is attained. The orientation-sum constraint is a modulo-three invariant, not a parity constraint.

## 1.4 Memory Cost

The original BFS explores all 3,674,160 valid cube states. Its main memory cost comes from the full-state move table, the BFS queue, and two transition tables:

```c
uint8_t *toward_solved = malloc(STATES);

uint32_t *queue =
    malloc((size_t) STATES * sizeof *queue);

uint16_t permutation[3][PERMUTATIONS];
uint16_t orientation[3][ORIENTATIONS];
```

| Buffer | Storage | Size calculation | Bytes |
| --- | --- | --- | ---: |
| Move table | Heap | 3,674,160 × 1 | 3,674,160 |
| BFS queue | Heap | 3,674,160 × 4 | 14,696,640 |
| Permutation transition table | Stack | 3 × 5,040 × 2 | 30,240 |
| Orientation transition table | Stack | 3 × 729 × 2 | 4,374 |
| **Total** | | | **18,405,414** |

These four buffers coexist during table construction and occupy approximately **17.553 MiB**, using 1 MiB = 1,048,576 bytes. The BFS queue accounts for approximately **79.8%** of this total and is the largest contributor.

After construction, the queue is freed and the local transition tables cease to be live when `build_table()` returns. The move table remains available for solving and occupies approximately **3.504 MiB** until it is freed.

These values are calculated from the buffer dimensions and element sizes. They exclude other variables, call-stack overhead, allocator overhead, and runtime memory, so they do not represent total process peak memory.

### Buffer Size Verification

To verify the calculated memory cost, size-reporting code was added to the original `build_table()` function. The array sizes were obtained using `sizeof`, while the heap allocation sizes were calculated from the allocation lengths and element sizes.

The program was compiled and executed with:

```bat
gcc -std=c99 -O2 -Wall -Wextra solver.c -o solver_baseline.exe
solver_baseline.exe 12345671111111
```

The output confirmed that the four major buffers total **18,405,414 bytes (17.553 MiB)** during table construction. The move table retained after construction occupies **3,674,160 bytes (3.504 MiB)**.

This verifies the buffer-size calculation; it does not measure total process peak memory.

![image](https://hackmd.io/_uploads/SyaRHtJjfl.png)



## 1.5 Instruction Cost

The memory sizes in Section 1.4 describe storage requirements. To characterize the baseline's execution cost, I counted the main operations performed by its full-state BFS.

The BFS removes all 3,674,160 states from the queue and examines nine moves for each state. Therefore, it performs:

$$
3{,}674{,}160 \times 9 = 33{,}067{,}440
$$

successor checks. Excluding the initial solved state, 3,674,159 newly discovered states are added to the queue.

Each successor check involves transition-table accesses, index calculations, and a visited-state test. When a previously unseen state is discovered, the BFS also records its inverse move in the move table and adds its rank to the queue. These operations require multiple machine instructions per successor check, making exhaustive table construction the main source of computational work.

The counts above characterize the algorithmic work of the BFS. They are not measured RV32I retired instruction counts, which depend on the compiled implementation. They also exclude the work required to initialize the buffers and construct the transition tables.

The baseline constructs the complete move table before solving the supplied input, even when that input is already solved. Replacing this preprocessing with smaller pattern databases and an input-directed search can reduce the amount of work required for an individual input.
 
## 1.6 Baseline Measurements

### Experimental Setup

I used Ripes `v2.2.6-106-g5b8a616` on the following host computer:

- Operating system: Windows Home, version 26H2, build 26300.9457 (64-bit)
- CPU: Intel Core i7-14650HX
- RAM: approximately 31.61 GiB of OS-addressable physical memory

The results below are individual measurements, not averages across multiple runs.

### Host-bytes-per-guest-byte Ratio

I measured host-memory overhead on the `RV32_ISS` model using two assembly programs, `memory_small.s` and `memory_4m.s`. They write a nonzero word to every four-byte location in guest-memory regions of 64 KiB and 4 MiB, respectively.

I executed each program in a separate Ripes CLI process and measured its peak host working-set size using PowerShell’s `PeakWorkingSet64`.

| Test Program | Guest Region Size (bytes) | Peak Host Working Set (bytes) |
|---|---:|---:|
| `memory_small.s` | 65,536 | 33,738,752 |
| `memory_4m.s` | 4,194,304 | 364,920,832 |

I estimated the additional host-memory cost per additional guest byte from the difference between the two measurements:

$$
R = \frac{H_{\text{4 MiB}} - H_{\text{64 KiB}}}
{4{,}194{,}304 - 65{,}536}
$$

$$
R = \frac{364{,}920{,}832 - 33{,}738{,}752}
{4{,}194{,}304 - 65{,}536}
\approx 80.21
$$

The resulting ratio was approximately **80.21 host bytes per guest byte**. This is an empirical result for the tested Ripes build and `RV32_ISS` model. It may vary between runs and processor models.

Using this measured ratio, the baseline's 18,405,414-byte buffer peak would correspond to approximately $18,405,414 \times 80.21 \approx 1.48 \times 10^9$ additional host bytes, or about 1.37 GiB. This is a rough extrapolation from the two memory-write probes, not a measured solver peak; it assumes similar per-byte overhead and excludes fixed simulator overhead.

Both assembly test files, `memory_small.s` and `memory_4m.s`, are available in my GitHub repository so that the experiment can be reproduced.

### Retired Instructions per Second

I measured simulator throughput by running the same `memory_4m.s` program on `RV32_ISS` and the standard five-stage pipeline model, `RV32_5S`.

I obtained the retired instruction count and wall-clock model execution time from the Ripes CLI using `--iret` and `--exectime`. The reported time was converted from milliseconds to seconds.

Throughput was calculated as:

$$
\text{Retired instructions per second}
= \frac{\text{Retired instruction count}}
{\text{Wall-clock model execution time in seconds}}
$$

| Processor Model | Retired Instructions | Wall-clock Model Execution Time (s) | Retired Instructions / Second |
|---|---:|---:|---:|
| `RV32_ISS` | 4,194,310 | 1.103 | 3,802,638 |
| `RV32_5S` | 4,194,310 | 15.269 | 274,694 |

Both models retired the same number of instructions, but their wall-clock execution times differed. For this test, `RV32_ISS` achieved approximately 13.84 times the throughput of `RV32_5S`.

These results measure simulator throughput on the host computer, rather than instructions per simulated clock cycle (IPC). They characterize Ripes using the memory-write test program and do not measure the execution cost of the cube solver. 

## 1.7 Problems Identified

The main problems of the baseline implementation are:

1. **High memory usage:** the BFS move table stores information for the complete state space.
2. **High preprocessing cost:** millions of states must be explored before solving the input state.
3. **Large amount of computational work:** the full-state BFS performs 33,067,440 successor checks, involving transition-table lookups, coordinate calculations, visited-state checks, and writes for newly discovered states.

These observations motivate the redesign in Stage 2.


# Stage 2 — Representation and Algorithm Design

The goal of Stage 2 is to choose a representation and search algorithm that better fit the target architecture while still guaranteeing an **optimal (shortest) solution**. The C implementation described in this stage is `vin_solver.c`.

## 2.1 Separate Permutation and Orientation

The original implementation already represents the seven movable corners using separate permutation and orientation arrays, `p[]` and `o[]`. I retained this representation and processed the two coordinates separately.

The permutation coordinate P identifies which corner occupies each position and has 5,040 possible values. The orientation coordinate O describes the corner twists and has 729 possible values, subject to the constraint that the total twist is divisible by three.

Under the existing move definitions, the updated permutation depends only on the current permutation and the selected move. Similarly, the updated orientation depends only on the current orientation and the selected move. Their transitions can therefore be computed separately. This does not imply that their solution lengths can be added.

The original implementation combines the coordinates into a full-state rank:

$$
rank = P \times 729 + O
$$

and constructs a move table over all 3,674,160 states. My implementation instead uses `rank_p()` and `rank_o()` to obtain the coordinate ranks directly and access separate distance tables.

The C search still retains a complete `state_t`, and every move updates both coordinates. The separate abstractions guide the full-state search rather than solve the cube independently.

## 2.2 Pattern Databases

I replaced the full-state move table with two pattern databases:

| Database | Abstract State | Stored Value | Size |
|---|---|---|---:|
| `p_distance` | Permutation, ignoring orientation | Minimum moves to the solved permutation | 5,040 bytes |
| `o_distance` | Orientation, ignoring permutation | Minimum moves to the solved orientation | 729 bytes |

Each database is constructed using BFS from the solved coordinate, whose rank is zero. All entries are initially set to `UINT8_MAX` to mark them as unvisited. The solved entry is assigned distance zero and placed in the queue.

During construction, the program generates quarter-turn transition tables for the R, B, and D faces. For each face, BFS applies its quarter-turn transition one, two, and three times to generate the three corresponding moves. Each resulting move has unit cost: for example, R, R2, and R' each count as one move, matching the baseline metric.

When an abstract state is first discovered, its distance is assigned as:

$$
distance[next] = distance[here] + 1
$$

Since BFS explores states in increasing distance and every move has unit cost, the databases contain exact shortest distances for their respective abstract problems.

The two retained distance tables require:

$$
5{,}040 + 729 = 5{,}769\text{ bytes}
$$

or approximately 5.634 KiB. This includes only the distance tables, excluding construction queues, transition tables, search storage, and other data.

Unlike the original `toward_solved` table, these databases store distances rather than next moves. IDA* must still search the complete cube state to produce a solution sequence.

Every complete solution must solve both coordinates, so its length cannot be smaller than either abstract shortest distance. The databases therefore provide lower bounds for the heuristic discussed in the next section.

## 2.3 Heuristic Function

The heuristic is defined as:

$$
h(s) = \max(h_p(s), h_o(s))
$$

where $h_p(s)$ is the shortest distance to the solved permutation while ignoring orientation, and $h_o(s)$ is the shortest distance to the solved orientation while ignoring permutation.

Let $d^*(s)$ be the optimal solution length for the complete cube. Any complete solution must solve both coordinates. Projecting that solution onto either abstract problem gives a valid sequence of the same length. Therefore:

$$
h_p(s) \leq d^*(s),
\qquad
h_o(s) \leq d^*(s)
$$

and consequently:

$$
h(s) = \max(h_p(s), h_o(s)) \leq d^*(s)
$$

The heuristic is therefore admissible: it never overestimates the optimal solution length.

I use the maximum rather than the sum because one move can change both coordinates. Adding the two distances could count overlapping work twice and overestimate the remaining number of moves.

In `vin_solver.c`, the search obtains the coordinates directly using `rank_p()` and `rank_o()`, accesses the corresponding distance-table entries, and selects the larger value.

## 2.4 IDA* Search

The solver uses Iterative Deepening A* (IDA*) to find a shortest solution without building a complete full-state move table.

For a node at depth $g$, the search evaluates:

$$
f(s) = g + h(s)
$$

The initial threshold is the heuristic value of the input state. During each depth-first search, a branch is pruned when:

$$
g + h(s) > \text{threshold}
$$

If the search fails, `ida_solve()` increases the threshold by one and restarts. The maximum threshold is 11, matching the diameter of the baseline state space under the same move metric.

The heuristic provides a lower bound, so this pruning cannot remove a solution whose length is within the current threshold. Since thresholds are tested in increasing order, the first successful search produces a shortest solution.

The search also skips moves on the same face as the previous move. Two consecutive moves on one face can be combined into a single supported move or cancel each other. Removing these redundant sequences preserves at least one shortest solution.

My first implementation used recursive calls. I later replaced them with an explicit stack:

```c
typedef struct {
    state_t state;
    uint8_t next_move;
    uint8_t previous_face;
} search_frame_t;
```

The current search uses `frames[12]` for depths 0 through 11 and `path[11]` to record the moves. Each frame stores the cube state, the next move to examine, and the previous face.

Before descending, the search advances the parent's move index and initializes the child frame. Backtracking decreases the depth index, allowing the parent to resume with its next candidate. This preserves the depth-first traversal without recursive function calls.

A state is recognized as solved when both its permutation rank and orientation rank are zero.

## 2.5 Why This Design Fits the Target

The baseline constructs a full-state move table before solving any input. Its four major construction buffers occupy 18,405,414 bytes, and the retained move table alone occupies 3,674,160 bytes.

The redesigned solver retains two pattern databases:

$$
5{,}040 + 729 = 5{,}769 \text{ bytes}
$$

This is approximately 5.634 KiB. These databases provide lower bounds rather than complete solutions for every cube state; IDA* still searches the complete state to determine the solution sequence.

The current C program constructs both databases at runtime using separate BFS procedures. Their transition tables and queues are temporary construction buffers. They are not retained by the search and must be counted separately when evaluating construction-time memory usage.

The search obtains P/O ranks directly, avoiding the earlier approach of combining them into a full-state rank and then dividing it again. I also replaced explicit runtime multiplication, division, and modulo operators with shifts, addition, subtraction, comparisons, and bounded software division routines. These changes prepare the C implementation for RV32I without the M extension. Their detailed implementation is discussed in Stage 3.

The C solver uses fixed-size arrays instead of heap allocation, and its explicit search stack removes recursion. However, these arrays currently use automatic storage in C; the assembly implementation must assign appropriate storage and account for it explicitly.

For the target version, the assignment permits transition and heuristic tables to be generated on the host and linked as read-only data. This provides a way to avoid constructing the pattern databases on the simulated processor while keeping the actual search on the target.

The main tradeoff is search time. IDA* can revisit states across paths and thresholds, so smaller tables do not automatically imply a lower instruction count.

The 5,769-byte figure covers only the distance tables. The final target's transition tables, search buffers, constants, and other static data must also be included when checking the 128 KiB limit. Compliance with the retired-instruction limit must be established separately through measurements of the final RV32I implementation.


# Stage 3 — C Optimization

After replacing full-state BFS preprocessing with P/O pattern databases and IDA*, I refined `vin_solver.c` by removing unnecessary coordinate conversions, replacing recursion with explicit search frames, and rewriting explicit runtime multiplication, division, and modulo operations.

The arithmetic and recursion changes are recorded in commits `f011828` and `8360357`. The search in this C implementation retains complete cubie arrays and computes coordinate ranks to access the heuristic tables.

## 3.1 Compute Coordinate Ranks Directly

The first IDA* implementation computed a combined state rank and then recovered the permutation and orientation coordinates through division and modulo. This conversion was unnecessary because the heuristic accesses separate P and O distance tables.

I introduced `rank_p()` and `rank_o()` to compute the required coordinates directly. The pattern-database builders also use the relevant ranking function when constructing their transition tables.

The combined `rank_state()` function remains available for state encoding and verification. It is no longer needed to obtain heuristic-table indices during search.

## 3.2 Replace Recursion with Explicit Search Frames

I replaced recursive search calls with a fixed array of 12 frames, representing depths 0 through 11. The solution moves are stored separately in an 11-entry path array.

In `vin_solver.c`, each frame contains the complete cube state, the next move to examine, and the previous face. Before descending, the search advances the parent's move index and initializes the child frame. Backtracking decreases the depth index, allowing the parent to resume from its next candidate.

The search retains heuristic pruning and consecutive-same-face pruning. Its frame storage is bounded, and it does not require recursion. The C frame array uses automatic storage; its placement in the assembly implementation is specified separately.

## 3.3 Rewrite Runtime Arithmetic for RV32I

RV32I does not provide the M extension. I therefore replaced explicit runtime multiplication, division, and modulo operators in the solver's arithmetic with shifts, comparisons, addition, and subtraction.

The changes cover:

- Small-factor multiplication during permutation ranking.
- Multiplication by three during orientation ranking.
- Multiplication by 729 when constructing a combined state rank.
- Orientation reduction modulo three.
- Move decoding and input indexing.
- Quotient and remainder calculations during unranking.

`divide_by_729()` and `divide_by_3()` perform software quotient-and-remainder calculations. Permutation unranking uses stored factorial values and repeated subtraction; the number of subtractions is bounded by the number of remaining cubies.

These are source-level changes. A compiler-generated RV32I reference must still be inspected for unsupported instructions and prohibited arithmetic helper calls. The host-only verification utilities are separate from the target solver and may use ordinary host arithmetic and allocation.

## 3.4 Correctness Verification

### Basic Self-Test

The `--self-test` mode checks that each move followed by its inverse restores the solved state. It also enumerates all 3,674,160 ranks, checks that each decoded state is valid, and confirms that re-encoding returns the original rank.

These checks validate state encoding and arithmetic changes, but they do not by themselves establish search correctness.

The host verifier also checked heuristic admissibility over the complete state space and confirmed that the P/O distance tables were populated. The maximum abstract distances were 7 for P and 6 for O, with both solved entries equal to zero.

### H3 — Exhaustive Verification of `vin_solver.c`

I performed complete host-side search verification of the supplied `vin_solver.c`. The verifier calls its existing iterative `ida_search()` function, using the same increasing-threshold procedure as `ida_solve()`.

A complete BFS based on the original baseline implementation provided the exact shortest distance for each state. For every state, the verifier checked:

1. The returned solution length equalled the exact BFS distance.
2. Replaying the returned move sequence with the original baseline's move functions restored both corner positions and orientations to the solved state.

| Search Implementation | States Checked | Replay Implementation | Result | Verification Time |
|---|---:|---|---|---:|
| `vin_solver.c` cubie-array IDA* | 3,674,160 | Original baseline move functions | PASS | 2,748.299 s |

All states passed both checks. The reported wall-clock time excludes BFS construction and setup. The verifier reported:

```text
PASS: 3674160 states; optimal lengths and baseline replay; 2748.299 wall seconds
```

The complete BFS table is used only for host verification and is not included in the RV32I program. This result verifies the tested C search; target-side assembly tests are reported separately in Stage 4.

## 3.5 Compiler Settings and Cost Comparison

The host verification program was compiled with GCC using `-std=c99 -O2 -Wall -Wextra`.

`-O2` enables compiler optimization. The reported H3 time measures exhaustive host verification, including search, length comparison, and solution replay. It is not an RV32I retired instruction count or the execution time for a single input.

The main implementation differences are:

| Operation | Earlier IDA* Implementation | Current `vin_solver.c` |
|---|---|---|
| Heuristic-table indexing | Combine P/O ranks, then divide and take modulo | Compute `rank_p()` and `rank_o()` directly |
| Constant multiplication | Explicit multiplication operators | Shifts, addition, and subtraction |
| Quotient and remainder during unranking | Division and modulo operators | Bounded software routines |
| Move decoding | Division and modulo by three | Comparisons and subtraction |
| Search traversal | Recursive function calls | Explicit frames and a depth index |

These comparisons describe source-level changes. The exhaustive verification establishes correctness, but its execution time alone does not demonstrate a speedup over the earlier implementation. A performance comparison requires identical inputs, build settings, and measurement methods.

The RV32I implementation and its code size, static-data size, retired instruction count, and correctness measurements are reported in Stage 4.

<!-- OPTIONAL C EXPERIMENT: excluded from the rendered main report.
To include it, remove this opening comment and the closing comment below.

## Optional Supplement — P/O Coordinate C Experiment

This is a separate experiment using `po_search.h`, `solver_po.c`, and `verify_po.c`. It does not replace the `vin_solver.c` implementation discussed in the main Stage 3 sections, and its results must not be attributed to `vin_solver.c`'s `ida_search()`.

The experimental search stores P/O ranks in each frame and obtains successors through separate nine-move transition tables. This avoids cubie-level move operations and repeated ranking at each search node. The heuristic, threshold progression, move order, and same-face pruning are preserved.

| Buffer | Storage |
|---|---:|
| P transition table | 90,720 bytes |
| O transition table | 13,122 bytes |
| P distance table | 5,040 bytes |
| O distance table | 729 bytes |
| **Total for these four buffers** | **109,611 bytes** |

The host program constructs these tables at startup. This total excludes frames, paths, temporary construction buffers, and other program data.

The experimental verifier checked all **3,674,160 states** against a saved complete BFS distance table and replayed every solution using my cubie-level `apply_move()` function. All optimal-length comparisons and replays passed in **477.411 wall-clock seconds**, excluding setup.

```text
PASS: 3674160 optimal lengths; 3674160 own replays; 477.411 wall seconds
```

Its BFS and replay share my own cube move definitions. The complete distance table is used only for host verification.

A separate single-run comparison of the same first 10,000 state ranks, with full replay in both programs and GCC `-O2`, gave:

| Search Implementation | Verification Time |
|---|---:|
| Cubie-array C IDA* | 8.952 s |
| P/O coordinate C IDA* | 1.530 s |

The coordinate version was approximately **5.85 times faster in this sample**. These are host verification times excluding setup, not target retired instruction counts. The complete 2,748.299-second and 477.411-second runs used different replay implementations, so their ratio is not a controlled measurement of the search change alone.

The assembly in Stage 4 uses P/O coordinate transitions. This experiment provides a corresponding host C search representation; the GCC-generated target reference is measured separately in Section 4.4.
-->

# Stage 4 — RV32I Implementation and Measurement

I implemented the P/O coordinate-based iterative IDA* design in RV32I assembly and evaluated it using Ripes `v2.2.6-106-g5b8a616`. Transition and heuristic tables are generated on the host and included as read-only data. Input validation, search, and solution replay execute on the simulated processor. The measurements exclude LED rendering.

## 4.1 RV32I Translation

### Coordinate-Based Search

The optimized C search in `vin_solver.c` stores a complete `state_t` in each frame. It applies moves to the cubie arrays and calculates the resulting permutation and orientation ranks.

The assembly search instead stores the two coordinate ranks directly. Each move updates them through separate transition tables:

$$
P' = T_P[P][move]
$$

$$
O' = T_O[O][move]
$$

This avoids updating the complete cubie arrays and recalculating their ranks at every search node.

The target contains transition entries for all nine moves. Each coordinate transition is stored as a 16-bit value, and the search reads it using `lhu`. Distance-table entries are stored as bytes and read using `lbu`.

The tables contain:

| Table | Entries | Bytes per Entry | Total Bytes |
|---|---:|---:|---:|
| Permutation transitions | 5,040 × 9 | 2 | 90,720 |
| Orientation transitions | 729 × 9 | 2 | 13,122 |
| Permutation distances | 5,040 | 1 | 5,040 |
| Orientation distances | 729 | 1 | 729 |
| **Total** | | | **109,611** |

These tables describe coordinate transitions and abstract distances. They do not contain a complete distance table for all 3,674,160 cube states.

### Iterative Search Frames

The assembly uses fixed arrays for iterative IDA*, covering depths 0 through 11:

- `search_coordinates`: 12 entries of 4 bytes, each storing a 16-bit P rank and a 16-bit O rank, for 48 bytes.
- `search_next_move`: one byte per depth, for 12 bytes.
- `search_previous_face`: one byte per depth, for 12 bytes.
- `solution_path`: up to 11 move bytes.

Descending stores the child coordinates and initializes its next-move and previous-face entries. Backtracking decreases the depth and moves the coordinate pointer back by four bytes and the next-move pointer back by one byte.

The final version keeps table base addresses in registers and uses the newly obtained P/O coordinates directly when checking a child node. This reduces repeated address calculations, loads, and function calls while preserving heuristic pruning and consecutive-same-face pruning.

The original 168-byte `search_states` buffer is retained, but only its first 14 bytes are used to back up the input for solution replay. Search successors are represented by coordinate ranks rather than complete cubie arrays.

### RV32I Arithmetic

Table addresses are calculated using shifts and additions. For example, each transition-table row contains nine 16-bit entries, so its size is 18 bytes:

$$
18P = 16P + 2P
$$

The assembly calculates this offset using shifts by four and one, followed by addition. It does not require multiplication or division instructions.

The binary audit found only base RV32I instructions, with no M, C, floating-point, or CSR instructions.

### Program Construction and Verification

`solver_core.s` contains the program, while the host C table generator produces `tables.s`. The rebuild script substitutes the input string, combines the sources into `solver_generated.s`, and assembles `solver_generated.elf`.

The `.section .rodata` declaration places the appended tables in a read-only section. The program does not contain a complete full-state distance table.

After finding a solution, the program restores the original input and replays its moves through the cubie-level `apply_move()` routine using `source_r`, `source_b`, `source_d` and their corresponding twist tables. It then checks that both `rank_p()` and `rank_o()` are zero.

This verifies the returned sequence through cubie-level operations rather than relying only on the coordinate search reaching rank zero. Expected move counts are checked by the test scripts; they are not embedded as an expected-length variable in the current assembly.

## 4.2 Test Data

I first tested three representative inputs:

| Test | Input State | Expected Optimal Moves | Result |
|---|---|---:|---|
| Solved cube | `12345671111111` | 0 | Passed |
| Short scramble | `25314672313211` | 1 | Passed |
| Required distance-11 vector | `21345671111111` | 11 | Passed |

The test scripts checked the expected solution length, and the program checked the cubie-level replay result before printing a solution.

I then tested all 2,644 distance-11 states on `RV32_ISS`. The input list was obtained from a host-side full-state BFS and checked against its distance table. This full-state table was used only as a testing oracle; it was not included in the target program.

The automated test script ran each input in a separate Ripes CLI process and required:

- A solution containing exactly 11 moves.
- Successful in-program replay verification.
- Successful solution output, with no search or replay failure.
- A recorded retired instruction count.

All 2,644 cases passed the correctness checks. The runner also independently replayed each printed path using cubie-level move definitions.

## 4.3 Ripes Measurements

I used the pinned Ripes build `v2.2.6-106-g5b8a616` with the `RV32_ISS` processor model.

Retired instructions were obtained using `--iret`. Counts include input parsing, coordinate ranking, search, solution replay, and text output. No LED renderer was included. There is no separate expected-length check in the current target program; the test scripts compare the reported length with the expected distance.

### Code and Static Data Size

The final handwritten ELF contains:

| Section | Bytes |
|---|---:|
| `.text` | 1,652 |
| `.data` | 455 |
| `.bss` | 0 |
| `.rodata` | 109,612 |
| **Total static data** | **110,067** |

Code size is the size of the linked `.text` section. Static data is calculated as:

$$
.data + .bss + .rodata
= 110{,}067 	ext{ bytes}
$$

The tables contain 109,611 bytes of entries, with one additional alignment byte in `.rodata`. Total static data is below the 128 KiB limit of 131,072 bytes, leaving 21,005 bytes of space. This figure does not include the simulator-provided stack or host process overhead.

### Retired Instruction Results

| Case | Input State | Retired Instructions |
|---|---|---:|
| Required vector | `21345671111111` | 16,947,844 |
| Highest count among all distance-11 inputs | `54721631111111` | 46,333,284 |

The complete distance-11 test produced:

| Check | Result |
|---|---:|
| Inputs tested | 2,644 |
| Correct 11-move solutions with successful replay | 2,644 |
| Missing retired instruction counts | 0 |
| Inputs exceeding 50,000,000 retired instructions | 0 |
| Maximum retired instruction count | 46,333,284 |
| Total batch wall-clock time | 668.542 s |

The batch time includes launching the test processes and performing verification, with four concurrent Ripes processes. It is not the simulation time of a single input or a sum of per-input execution times.

The maximum count is below the limit by:

$$
50{,}000{,}000 - 46{,}333{,}284
= 3{,}666{,}716
$$

This leaves approximately 7.33% of the instruction budget unused in the worst tested distance-11 case.

The template ELF used by the complete test has the following SHA-256 hash:

```text
67473929dfbc499228e17bc0da1803671b053b48289a3d4ba683cb33252538b6
```

The script changes only the input string in temporary ELF copies while preserving the instruction code and lookup tables. Individual input ELFs therefore have different hashes. Changes to the code or tables require renewed validation and measurement.

### Assembly Refinement

The earlier coordinate-table revision repeatedly calculated frame addresses and called helper routines during search. The final revision retains table bases and frame pointers in registers and performs the frequent lookups directly in the search loop.

| Input | Earlier Coordinate Assembly | Final Coordinate Assembly |
|---|---:|---:|
| `21345671111111` | 24,902,514 | 16,947,844 |
| `54721631111111` | 68,086,180 | 46,333,284 |

Both columns are `RV32_ISS` retired instruction counts with rendering excluded and solution replay and output included. The reductions are approximately 31.94% and 31.95%, respectively. The earlier difficult case exceeded the limit; the final revision passed the complete 2,644-case test.

The final linked `.text` size is 1,652 bytes. An earlier linked `.text` measurement was not retained for this comparison, so no code-size reduction is claimed.

## 4.4 Final Comparison

The main implementations differ in both algorithm and representation. Here, optimized C refers specifically to `vin_solver.c`:

| Aspect | Original C Baseline | Optimized C (`vin_solver.c`) | Current RV32I Assembly |
|---|---|---|---|
| Solution method | Full-state BFS preprocessing, followed by move-table lookup | P/O pattern databases and IDA* | P/O pattern databases and IDA* |
| Search-state representation | Combined P/O rank during BFS | Complete cubie arrays | Separate P/O coordinate ranks |
| Heuristic | Not used | Maximum of two abstract distances | Maximum of two abstract distances |
| Move generation | Coordinate transition tables during BFS; cubie-level moves when following the solution | Cubie-level moves during IDA* | Nine-move coordinate transition tables during IDA*; cubie-level moves for verification |
| Search traversal | BFS queue | Explicit depth-first search frames | Fixed coordinate arrays and depth pointers |
| Table construction | Runtime full-state BFS and transition-table construction | Runtime abstract BFS and transition-table construction | Host-generated tables included as read-only data |

### Memory Comparison

The original baseline's four major construction buffers occupy 18,405,414 bytes. After construction, its retained move table occupies 3,674,160 bytes.

`vin_solver.c` retains 5,769 bytes of distance tables during search. This figure excludes temporary construction queues, transition tables, search frames, and other storage.

The RV32I assembly contains 110,067 bytes of total static data, including nine-move transition tables, distance tables, and fixed program storage. Its static-data size is below the 128 KiB limit.

These values describe different memory scopes. They show how storage is organized in each implementation, but they are not equivalent measurements of total process peak memory.

### Search and Instruction Cost

The baseline examines 33,067,440 successors during full-state BFS preprocessing, regardless of the supplied input.

Both redesigned implementations use IDA* to search from the supplied state. Their pattern-database heuristic prunes branches that cannot produce a solution within the current threshold.

The assembly additionally uses coordinate transition tables to update P/O ranks directly. This avoids the cubie-array updates and repeated ranking performed at each search node in `vin_solver.c`.

All 2,644 distance-11 inputs produced correct 11-move solutions with successful replay on `RV32_ISS`. The maximum retired instruction count was 46,333,284, below the 50,000,000 limit. The required vector `21345671111111` retired 16,947,844 instructions.

### GCC-Generated RV32I Reference

For the compiler comparison, `solver_reference.c` implements the final coordinate-based IDA* algorithm. It uses the same four precomputed tables, move order, increasing thresholds, heuristic, same-face pruning, and cubie-level solution replay as the assembly. It is separate from the earlier cubie-array `vin_solver.c`.

The installed compiler was `riscv32-unknown-elf-gcc` 11.1.0, with:

```text
-O2 -march=rv32i -mabi=ilp32
```

The assignment names `riscv64-unknown-elf-gcc`; this experiment used the installed `riscv32-unknown-elf-gcc` targeting RV32I. The toolchain name and version are recorded explicitly. These results should not be presented as a measurement of a different compiler executable or version.

The additional build options were:

```text
-std=c99 -ffreestanding -fno-builtin -nostdlib -nostartfiles
-msmall-data-limit=0 -Wl,--no-relax -Wl,-T,link.ld
```

These options provide a bare-metal Ripes executable without a standard C library or implicit arithmetic helpers. A small assembly entry initializes a fixed stack, calls the C program, and exits. Inline `ecall` wrappers provide console output. The initial permutation ranking uses explicit shift-and-add expressions to prevent GCC from turning a repeated-addition loop into a prohibited `__mulsi3` call.

The comparison script checked that all four table contents were identical in the two ELFs and audited the emitted instructions for base RV32I. Both versions were run on the same Ripes build and `RV32_ISS` model with rendering excluded. Counts cover startup, parsing, ranking, search, replay, and output, rather than search alone.

The following comparison measurements were executed by Codex on the same host using `build_and_compare.ps1` / `compare.py`; they are recorded in `comparison.json` and were not separately rerun manually.

| Input State | Expected Moves | Handwritten RV32I Instructions | GCC `-O2` Instructions |
|---|---:|---:|---:|
| `12345671111111` | 0 | 1,310 | 838 |
| `25314672313211` | 1 | 2,158 | 1,337 |
| `63157423133333` | 6 | 4,562 | 2,889 |
| `21345671111111` | 11 | 16,947,844 | 13,201,743 |
| `54721631111111` | 11 | 46,333,284 | 36,093,921 |

All five cases passed the expected-length checks and independent replay checks, and both implementations produced the same tested move sequences.

| Size Measurement | Handwritten RV32I | GCC `-O2` Reference |
|---|---:|---:|
| Linked `.text` | 1,652 bytes | 1,400 bytes |
| `.data + .bss + .rodata` | 110,067 bytes | 110,975 bytes |

The GCC static-data total includes a fixed 1,024-byte startup stack. The handwritten ELF instead uses the stack supplied by the simulator, so these storage arrangements differ. Both totals are below 128 KiB.

GCC retired fewer instructions on all five tested inputs and generated a `.text` section 252 bytes smaller. The handwritten version therefore does not currently outperform this GCC reference in either tested instruction count or code size. Handwritten assembly does not automatically produce a better result: the remaining move decoding, loop branches, and supporting routines are opportunities for further comparison and refinement. The individual contribution of each was not measured here.

The handwritten version's complete 2,644-case test establishes its distance-11 budget compliance. The five-case GCC comparison does not establish a complete-domain correctness result or an all-distance-11 performance bound for the GCC version.

Section 4.6 reports the three successful pipeline solver tests, explains the instruction stages, and records the GUI load/store observations. The throughput experiment in Section 1.6 is separate from these solver tests.

## 4.5 LED Matrix Visualization

**TODO — Not implemented in this submission.** The current program provides console output only; all reported measurements exclude LED rendering.

## 4.6 Pipeline Verification

The same handwritten solver ELF was tested on `RV32_ISS` and the standard five-stage pipeline model, `RV32_5S`, using Ripes `v2.2.6-106-g5b8a616`. Rendering was excluded. Counts include parsing, search, cubie-level replay, and console output.

| Input State | Expected Moves | `RV32_ISS` Instructions | `RV32_5S` Instructions | `RV32_5S` Model Time | Result |
|---|---:|---:|---:|---:|---|
| `12345671111111` | 0 | 1,310 | 1,309 | 0.005 s | PASS |
| `25314672313211` | 1 | 2,158 | 2,157 | 0.008 s | PASS |
| `21345671111111` | 11 | 16,947,844 | 16,947,843 | 51.641 s | PASS |

Both models produced the expected solution lengths and the same tested move sequences. The program performed cubie-level replay before printing a solution, and the runner independently replayed each printed path. The pipeline tests were executed by Codex on the same host. Their results are recorded in `summary.json` in the pipeline-verification evidence package. Times are individual wall-clock model measurements, not simulated cycle counts or averages.

The pipeline reported one fewer retired instruction in each case. The raw counts are preserved; the cause of this reporting difference was not verified. The complete 2,644-case instruction-budget result in Section 4.3 uses `RV32_ISS`, as required.

### Instruction-Level Explanation

The following fragment is from the actual permutation-transition lookup. `t1` holds the parent P rank, `s6` the selected move, and `s10` the table base:

```asm
slli t3, s6, 1
slli t4, t1, 4
slli t5, t1, 1
add t4, t4, t5
add t4, t4, t3
add t4, t4, s10
lhu t4, 0(t4)
```

It calculates `p_transition + 18 * P + 2 * move` using shifts and additions, then loads the child P rank. The offset is correct because each parent has nine moves and each entry occupies two bytes.

For `lhu t4, 0(t4)`, the five stages perform the following operations:

| Stage | Operation |
|---|---|
| IF | Fetch the instruction at the current PC. |
| ID | Decode a halfword load and identify `t4` as both the address-source register and destination register. |
| EX | Calculate the effective address from the source `t4` value plus the immediate zero. |
| MEM | Read the 16-bit table entry and zero-extend it. The table is not modified. |
| WB | Write the loaded child rank to `t4`. The write-back selection chooses memory data, and register write enable is active. |

The address operand selection uses the immediate for the load's ALU calculation. A dependency on the preceding address calculation must be handled by the processor's forwarding and hazard logic; no exact stall count was measured here.

Later, `sh t4, 0(s8)` saves that rank in the child coordinate frame. Its EX stage calculates the destination address, and its MEM stage writes the low 16 bits of `t4`. The store enables a memory write and does not request a destination-register write. The processor-wide register write signal may still be active for a different instruction simultaneously in WB. The next instruction stores O at `2(s8)`, so the two ranks occupy a four-byte coordinate entry.

The stage explanation above is supported by the following GUI stepping observations supplied during the walkthrough. The CLI tests separately establish the three-case pipeline execution results.

### GUI Stepping Observations

I used the short input `25314672313211` with the standard five-stage processor and RV32I. The following observations were recorded from the Ripes GUI screenshots and the register check made during single stepping.

| Cycle | Selected Instruction | Stage | Observation |
|---|---|---|---|
| 1,145 | `0x4b4: lhu x29, 0(x29)` | IF | The load was fetched. `t4` still contained the earlier intermediate value `0x00004500`. |
| 1,149 | `0x4b4: lhu x29, 0(x29)` | WB | Register write enable was green. The register panel still showed the source address `0x1001642a` before the next clock edge. |
| Next step | Load write-back completed | — | The register check confirmed `x29/t4 = 0x00000cde`, or P rank 3,294. |
| 1,161 | `0x4e8: sh x29, 0(x24)` | MEM | Data memory write enable was green, showing that the child P rank was being stored. |

![download](https://hackmd.io/_uploads/BJN3JGrofx.png)
> Figure 1. The transition-table load enters the IF stage.

![download](https://hackmd.io/_uploads/H15TyMSjMe.png)
> Figure 2. The load reaches WB with register write enable active. The register panel still shows the address before the next clock edge.

The linked table base is `0x1001168a`. For the initial P rank 1,104 and candidate move R, the entry address is `0x1001168a + 1104 * 18 = 0x1001642a`, matching the address seen before load write-back. The stored table value is 3,294, matching the observed load result.

At cycle 1,161, `addi x24, x24, 4` was simultaneously in WB while the store was in MEM. The register panel still showed the earlier `s8` value `0x10000120`. The store's architectural destination is the child entry at `0x10000124`; this dependency is handled by the pipeline rather than requiring the visible register panel to update first. Forwarding selector values were not captured, so no particular selector encoding is claimed.


Register write enable was also green in this store screenshot because the preceding `addi` was in WB. That signal belongs to the instruction currently writing back; it does not mean that `sh` writes a destination register. The store performs its update through the separate data-memory write enable.
![download](https://hackmd.io/_uploads/BkgHZxGSszg.png)
> Figure 3. The store reaches MEM with data-memory write enable active. The preceding addi is simultaneously in WB.

At cycle 1,162, the next screenshot showed `s8 = 0x10000124`. The Memory viewer screenshot confirms the stored result: bytes `0xde` and `0x0c` were present at `0x10000124` and `0x10000125`. In little-endian order these represent `0x0cde`, or P rank 3,294, confirming that the loaded table result was saved to the child coordinate entry. The screenshots show the load's IF and WB stages and the store's MEM stage; the intermediate load stages are explained above rather than claimed as separately captured images.

![download](https://hackmd.io/_uploads/ByYvlGBoMg.png)

> Figure 4. The Memory viewer confirms that the child P rank, 3,294 (`0x0cde`), was stored at `0x10000124` as bytes `0xde` and `0x0c` in little-endian order.

## Assembly Development — Design and Execution Flow

The assembly was developed in small steps: input parsing and validation, cubie moves, coordinate ranking, heuristic lookup, iterative search, and solution verification. The final search uses P/O transition tables, while the cubie move routines are retained for replay.

### 1. Input Parsing

The input is stored as a 14-character null-terminated string. The parser subtracts ASCII `'1'` from each character and writes the resulting value into a 14-byte state buffer.

The first seven bytes store corner permutation values, and the remaining seven bytes store corner orientations. Source and destination pointers advance by one byte after each character, while a counter decreases from 14 to zero.

### 2. Input Validation

The parser checks the following conditions:

1. **Length:** Exactly 14 characters, followed by a null terminator.
2. **Permutation range:** P values must be between 0 and 6.
3. **Permutation uniqueness:** A bit mask records the corners already seen and rejects duplicates.
4. **Orientation range:** O values must be between 0 and 2.
5. **Orientation constraint:** The sum of the seven O values must be divisible by three. Repeated subtraction calculates the remainder without division.

Invalid input exits with status `1`. Valid input proceeds to search. The earlier input tests covered valid states, incorrect lengths, out-of-range values, duplicates, and an invalid orientation sum.

### 3. R Turn

The R turn uses `source_r` to select which old corner enters each new position and `twist_r` to update its orientation. If the new orientation is at least three, the routine subtracts three.

The result is written into `next_state`, so reading the original state is not affected by partially written results. I checked the single-turn values, four-turn restoration, and R followed by R′. R′ is implemented as three quarter-turns.

### 4. B and D Turns

B and D share the same turn loop but use their own source and twist tables. D changes corner positions without adding an orientation twist. Single-turn values, four-turn restoration, and inverse-turn restoration were tested.

### 5. Apply a Move

`apply_move()` accepts move numbers 0 through 8, corresponding to R, R2, R′, B, B2, B′, D, D2, and D′.

Repeated subtraction by three determines the face. The remaining value plus one gives the number of quarter-turns. After each turn, `copy_state()` copies `next_state` back to `state`.

Because the routine calls other routines, it saves its return address and the `s0` and `s1` registers it uses, then restores them before returning. This uses a bounded call stack without recursion.

### 6. Encode P and O

`rank_p()` counts smaller corner values to the right of each position and accumulates a permutation rank from 0 to 5,039. Its small-factor multiplication is performed by repeated addition.

`rank_o()` encodes the first six orientations as a base-three value from 0 to 728. Each step calculates `3 * o + O[i]` using a shift and addition. The seventh orientation is determined by the validated orientation-sum constraint.

The input is ranked once before search. The final search does not reconstruct and rank cubie arrays at every node.

### 7. Heuristic and Host-Generated Tables

The heuristic is `max(p_distance[P], o_distance[O])`. These abstract distances are lower bounds on the complete solution length, as explained in Section 2.3.

The host C tool generates the two distance tables and nine-move P/O transition tables. The rebuild script appends them as read-only data and produces the ELF. The target still performs the search itself.

During search, a move produces its child coordinates through table lookups. Each transition row contains nine two-byte entries, so its address is `table_base + 18 * rank + 2 * move`, calculated with shifts and additions.

### 8. Iterative IDA* Search

The main search uses `s3` for the current depth and `s4` for the limit. P/O ranks are stored in `search_coordinates`; `search_next_move` and `search_previous_face` preserve the information needed to resume a parent node.

The execution flow is:

```text
Parse and validate input
    → Back up the original cubie state
    → Rank P and O; set limit to the initial heuristic
    → Check g + h and whether the coordinates are solved
    → Try the next move, skipping a consecutive move on the same face
    → Look up child P/O and descend
    → Backtrack when pruned or when all moves have been tried
    → Increase the limit and restart if necessary, up to 11
    → Replay and verify the first solution
    → Print the solution and exit
```

The parent's next-move value is advanced before descending. Backtracking therefore resumes at its next candidate rather than trying the same move again. Search depth is managed explicitly; the routine does not recursively call itself.

### 9. Reduce Repeated Work

The final revision retains the distance-table bases in `s0` and `s1`, and transition-table bases in `s10` and `s11`. During search, `s8` points to the current coordinate entry and `s9` points to the current next-move byte.

Descending advances these pointers by four bytes and one byte; backtracking reverses those changes. Child coordinates remain in `s2` and `s5` for the immediate heuristic check, reducing unnecessary loads and helper calls. When a parent resumes, its coordinates are read from its saved entry.

Once a solution is found, `s8` and `s9` are reused as the replay index and solution length. The search pointers are no longer needed at that point. The original 168-byte `search_states` allocation remains, but only its first 14 bytes are used to back up the input for replay.

### 10. Replay, Output, and Final Tests

After search, the program restores the original input and applies every move from `solution_path` using the cubie-level routines. It checks that both coordinate ranks are zero before printing the solution and exiting with status `0`.

The final revision passed all 2,644 distance-11 tests on `RV32_ISS`; the maximum count was 46,333,284 instructions. The solved, short-scramble, and required distance-11 cases also passed on `RV32_5S`. The GCC comparison is reported in Section 4.4; the handwritten version currently uses more instructions and code bytes than that reference.

LED rendering is not implemented. Pipeline execution has been tested, and the instruction-level explanation is included in Section 4.6. GUI stepping observations for the transition load and coordinate store are recorded in Section 4.6.


## Repository and Reproduction

This project started from [sysprog21/minirubik at 231796c](https://github.com/sysprog21/minirubik/tree/231796cc48868f4ea276f652139b6bebbad0cd02), the parent of my first project-specific commit. My work is recorded in [the repository history](https://github.com/x29740736x/minirubik/commits/main/).

- [RV32I source and build tools](rv32i/README.md)
- [Host verification sources and evidence limitations](tests/VERIFICATION.md)
- [Complete distance-11 results](tests/distance11/summary.json)
- [GCC comparison](gcc-comparison/README.md)
- [Pipeline measurements](tests/pipeline/summary.json)

The original full cubie-array H3 JSON/log were overwritten by subsequent incomplete reruns. The PASS line reported above is retained from terminal output, rather than a surviving full-run JSON. See tests/VERIFICATION.md. The coordinate verification is a separate implementation.

## References

- [Assignment 1 specification](https://hackmd.io/@sysprog/2026-arch-homework1)
- [Original minirubik source](https://github.com/sysprog21/minirubik/tree/231796cc48868f4ea276f652139b6bebbad0cd02)
- [Ripes CLI documentation](https://github.com/mortbopet/Ripes/blob/master/docs/cli.md)
- [Ripes environment calls](https://github.com/mortbopet/Ripes/blob/master/docs/ecalls.md)
