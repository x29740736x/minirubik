param(
    [string]$AssemblyElf = 'handwritten.elf',
    [string]$GccExe = 'C:\embeetle\embeetle\beetle_tools\gnu_riscv_toolchain_11.1.0_64b\bin\riscv32-unknown-elf-gcc.exe',
    [string]$RipesExe = 'C:\Users\vivi2\OneDrive\桌面\Ripes-v2.2.6-106-g5b8a616-win-x86_64\Ripes.exe'
)
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$pythonExe = 'C:\Users\vivi2\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
if (-not (Test-Path -LiteralPath $GccExe)) { throw 'RISC-V GCC not found. Supply -GccExe with its full path.' }
& $GccExe --version
$compilerArgs = @('-std=c99', '-O2', '-march=rv32i', '-mabi=ilp32',
    '-ffreestanding', '-fno-builtin', '-nostdlib', '-nostartfiles',
    '-msmall-data-limit=0', '-Wl,--no-relax', '-Wl,-T,link.ld',
    'start.s', 'solver_reference.c', 'tables_gnu.s', '-o', 'gcc_reference.elf')
& $GccExe @compilerArgs
if ($LASTEXITCODE -ne 0) { throw 'GCC reference build failed.' }
& $pythonExe .\compare.py --assembly $AssemblyElf --ripes $RipesExe
if ($LASTEXITCODE -ne 0) { throw 'Comparison failed. Inspect the console and test files.' }
Get-Content -LiteralPath .\comparison.json
