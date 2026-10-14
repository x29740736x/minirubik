# RV32I solver with LED visualization

## Files

- `solver_core.s`: search, replay and guarded LED routines; edit this file.
- `tables.s`: the existing host-generated P/O distance and transition tables.
- `solver_led_gui.s`: complete GUI source including tables, ready for Ripes.
- `build.py`: selects the GUI or renderer-disabled CLI build.
- `assemble_coordinate.py`: restricted RV32I assembler used by the CLI build.

The GUI-ready source intentionally has no `.if` or `.equ RENDER` directives.
The canonical core retains the guards, and build.py resolves them for the selected
mode. `--mode` is authoritative; changing only the core's RENDER value does not
override the build.py mode. The two modes differ only by renderer inclusion.

## GUI execution

1. Add an LED Matrix in Ripes I/O, Width 35 and Height 25.
2. Load `solver_led_gui.s` as **Source file**, not ELF.
3. Confirm the disassembly starts with input parsing, not the old LED test loop.
4. Reset and run. The solver draws the input after finding a solution and redraws
   after each actual solution move. Final cubie replay must pass before output.

Each sticker occupies 4x3 LEDs. Six face slots form the U / L-F-R-B / D net.
LED addresses use peripheral symbols and row-major indexing. The delay counter is
1000 iterations; this is not a fixed real-time duration.

The default input is `21345671111111`, the user's tested 11-move case.
To change it or regenerate the GUI source, from this directory:

```powershell
python .\build.py --mode gui --input 25314672313211
```

Use the same pinned Ripes build as the original report. If source and disassembly
do not match, reopen Ripes and reload the source, as in the successful GUI test.

## Non-LED measurement build

```powershell
python .\build.py --mode cli --input 21345671111111
```

This produces `solver_cli.elf` with no renderer data, calls, or MMIO references.
Use RV32_ISS and the existing measurement/test procedure. The original `../rv32i`
and its recorded test results remain unchanged. New ELF fingerprints depend on
the input; do not attribute old measurements to a modified search implementation.

## Test status

The user confirmed standalone R/B/D four-turn and inverse restoration, P/O decode
agreement, the one-move R solve, and the 11-move GUI replay returning to solved
faces. The reorganization additionally checks that the generated GUI text matches
the working source and the non-LED ELF matches a rebuild of the original solver
with the same input. This is not a new full-domain or GUI execution test.

`tables.s` is preserved from the existing build; this script does not regenerate
it. The original table-generation tools remain in `../rv32i`.
