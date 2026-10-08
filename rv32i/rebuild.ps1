param([string]$InputState = '25314672313211')
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
if ($InputState -notmatch '^[1-7]{14}$') { throw 'Input must be 14 digits; full validity is checked by the assembly program.' }

#本機編譯建表工具，不是編譯RV32I搜尋程式
$compiler = (Get-Command gcc -ErrorAction Stop).Source
& $compiler -std=c99 -O2 -Wall -Wextra build_tables.c -o build_tables.exe
if ($LASTEXITCODE -ne 0) { throw 'Table generator compilation failed' }
& .\build_tables.exe
if ($LASTEXITCODE -ne 0) { throw 'Table generation failed' }

#保留core，另外產生包含完整表格的組語
$core = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'solver_core.s'))
$tables = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'tables.s'))
$pattern = [regex]::new('\.asciz "[1-7]{14}"')
$core = $pattern.Replace($core, '.asciz "' + $InputState + '"', 1)
$combined = $core + "`r`n" + $tables
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'solver_generated.s'), $combined, [Text.UTF8Encoding]::new($false))

#組譯為Ripes可載入的ELF
$pythonPath = 'C:\Users\vivi2\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
if (-not (Test-Path -LiteralPath $pythonPath)) { $pythonPath = (Get-Command python -ErrorAction Stop).Source }
& $pythonPath assemble_coordinate.py solver_generated.s solver_generated.elf
if ($LASTEXITCODE -ne 0) { throw 'Assembly failed' }
Write-Host "Built solver_generated.elf for $InputState"
