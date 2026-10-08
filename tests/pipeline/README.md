# Five-stage pipeline verification

The recorded summary covers the solved, one-move and required 11-move inputs on
RV32_5S. Output paths were independently replayed by the Python checker; the
assembly also performs its own replay. These automated measurements were made
by Codex on the development host. GUI observations are described in report.md.

From this directory:

```powershell
python .\test_pipeline.py --elf ..\..\rv32i\solver_generated.elf --ripes 'PATH\TO\Ripes.exe'
```