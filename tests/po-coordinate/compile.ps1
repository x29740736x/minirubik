$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$compilerPath = (Get-Command gcc -ErrorAction Stop).Source
foreach ($name in @('solver_po','verify_po','build_table')) {
    & $compilerPath -std=c99 -O2 -Wall -Wextra "$name.c" -o "$name.exe"
    if ($LASTEXITCODE -ne 0) { throw "$name compilation failed" }
}
[ordered]@{
    compiler = $compilerPath
    compiler_version = (& $compilerPath --version | Select-Object -First 1)
    flags = '-std=c99 -O2 -Wall -Wextra'
    sources = @(Get-FileHash -Algorithm SHA256 vin_solver.c,po_search.h,solver_po.c,verify_po.c,build_table.c | Select-Object Path,Hash)
    executables = @(Get-FileHash -Algorithm SHA256 solver_po.exe,verify_po.exe,build_table.exe | Select-Object Path,Hash)
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath build_info.json -Encoding utf8
Write-Host 'All programs compiled with explicit -O2'
