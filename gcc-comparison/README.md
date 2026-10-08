# GCC comparison

This package compares the current stepwise handwritten solver with a C coordinate-search reference compiled by GCC 11.1.0. It does not compare against the original runtime BFS or the earlier cubie-array IDA*.

Both programs use identical P/O distance and nine-move transition tables, the same R/R2/R'/B/B2/B'/D/D2/D' move order, maximum-of-two heuristic, increasing bounds, same-face pruning and cubie-level replay. The runner checks table bytes in both ELFs and independently replays the printed paths. Rendering is excluded. Counts cover program startup, parsing, ranking, search, replay and text output; they are not search-only measurements.

## Run on this machine

Extract the package, open PowerShell in its directory and run:

```powershell
.\build_and_compare.ps1
```

The included `handwritten.elf` is a copy of the submitted working ELF with SHA-256 `67473929dfbc499228e17bc0da1803671b053b48289a3d4ba683cb33252538b6`. To measure a later assembly build, pass `-AssemblyElf` with its absolute path. The runner changes only the 15 input-string bytes in temporary copies; it does not modify the original ELF.

## Compiler

The installed executable is `riscv32-unknown-elf-gcc`, GCC 11.1.0, rather than the assignment's named `riscv64-unknown-elf-gcc`. Both the actual executable name and version must be reported. Target options are `-O2 -march=rv32i -mabi=ilp32`. This is a GCC-generated RV32I reference; it is not evidence that the differently named/versioned compiler produces identical output. The script accepts `-GccExe` for a toolchain using the assignment's exact executable name.

Additional options: `-std=c99 -ffreestanding -fno-builtin -nostdlib -nostartfiles -msmall-data-limit=0 -Wl,--no-relax -Wl,-T,link.ld`. These provide a bare-metal Ripes executable without host libc, implicit arithmetic helpers, or global-pointer startup dependencies. `start.s` initializes a fixed 1024-byte stack, calls the C main, and exits via ecall. `link.ld` specifies memory placement. Small inline ecall wrappers implement Ripes printing. No LED code is present.

The C reference implements the final algorithm and produces the same tested move sequences; it is newly prepared comparison source, not the unmodified earlier `vin_solver.c`. Its initial P ranking uses explicit shifts/adds for small constant products because GCC folded the original bounded addition loop into a prohibited `__mulsi3` call. The final build links without libraries, and the runner audits base RV32I instructions and identical table contents.

## Files

- `solver_reference.c`: the C solver compiled for RV32I.
- `tables_gnu.s`: the same table values, using GNU `.2byte` and `.balign` directives.
- `start.s`, `link.ld`: bare-metal entry and section placement.
- `build_and_compare.ps1`, `compare.py`: reproducible build and five-case comparison.
- `comparison.json`: measured results and binary fingerprints.
- `.elf` files and disassembly: exact test artifacts.

Linked `.text` sizes: handwritten 1652 bytes, GCC 1400 bytes. GCC currently wins in both code size and retired instructions for the five tested cases. Neither a full-domain GCC correctness claim nor an all-distance-11 GCC performance claim follows from these five cases. The handwritten all-2644 result remains the separate previously completed test.
