$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$compilerPath = (Get-Command gcc -ErrorAction Stop).Source
& $compilerPath -std=c99 -O2 -Wall -Wextra build_table.c -o build_table.exe
if ($LASTEXITCODE -ne 0) { throw 'build_table compilation failed' }
& $compilerPath -std=c99 -O2 -Wall -Wextra verify_again.c -o verify_again.exe
if ($LASTEXITCODE -ne 0) { throw 'verify_again compilation failed' }
[ordered]@{
    compiler = $compilerPath
    compiler_version = (& $compilerPath --version | Select-Object -First 1)
    flags = '-std=c99 -O2 -Wall -Wextra'
    sources = @(Get-FileHash -Algorithm SHA256 vin_solver.c,build_table.c,verify_again.c | Select-Object Path,Hash)
    executables = @(Get-FileHash -Algorithm SHA256 build_table.exe,verify_again.exe | Select-Object Path,Hash)
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath build_info.json -Encoding utf8
Write-Host 'Both programs compiled with explicit -O2; recorded in build_info.json'
