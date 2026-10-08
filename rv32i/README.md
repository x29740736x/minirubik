# RV32I solver

`solver_core.s` contains the handwritten search. The host generator uses the
adjacent vin_solver.c snapshot and po_search.h to emit the four abstract tables.
`solver_generated.s` combines the core and tables. The checked-in ELF has SHA-256
67473929dfbc499228e17bc0da1803671b053b48289a3d4ba683cb33252538b6 and matches
the complete 2,644-case test in ../tests/distance11/summary.json.

From this directory, with host GCC and Python installed:

```powershell
.\rebuild.ps1 -InputState '54721631111111'
```

Rebuilding changes the embedded input and may change the ELF fingerprint. Keep
test results associated with their recorded ELF. Load solver_generated.elf as
Executable (ELF) in Ripes using RV32I. LED rendering is not implemented.

To repeat the distance-11 test, from tests/distance11:

```powershell
python .\test_distance11.py --elf ..\..\rv32i\solver_generated.elf --ripes 'PATH\TO\Ripes.exe' --workers 4
```