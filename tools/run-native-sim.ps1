param(
    [switch]$BuildOnly
)

$ErrorActionPreference = 'Stop'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw 'Visual Studio Installer (vswhere.exe) was not found.'
}

$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    throw 'Visual Studio C++ build tools were not found.'
}

$vcToolsVersionFile = Join-Path $vsPath 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt'
$vcToolsVersion = (Get-Content $vcToolsVersionFile -Raw).Trim()
$vcTools = Join-Path $vsPath "VC\Tools\MSVC\$vcToolsVersion"
$cl = Join-Path $vcTools 'bin\Hostx64\x64\cl.exe'

$sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
$sdkVersion = Get-ChildItem (Join-Path $sdkRoot 'Include') -Directory |
    Where-Object { Test-Path (Join-Path $_.FullName 'ucrt') } |
    Sort-Object { [version]$_.Name } -Descending |
    Select-Object -First 1 -ExpandProperty Name
if (-not $sdkVersion) {
    throw 'A Windows 10/11 SDK installation was not found.'
}

$env:PATH = "$(Split-Path $cl);$env:PATH"
$env:INCLUDE = @(
    (Join-Path $vcTools 'include'),
    (Join-Path $sdkRoot "Include\$sdkVersion\ucrt"),
    (Join-Path $sdkRoot "Include\$sdkVersion\um"),
    (Join-Path $sdkRoot "Include\$sdkVersion\shared")
) -join ';'
$env:LIB = @(
    (Join-Path $vcTools 'lib\x64'),
    (Join-Path $sdkRoot "Lib\$sdkVersion\ucrt\x64"),
    (Join-Path $sdkRoot "Lib\$sdkVersion\um\x64")
) -join ';'

$root = Split-Path $PSScriptRoot -Parent
$source = Join-Path $root 'test\test_sim.cpp'
$output = Join-Path $root '.pio\build\native_sim\tide_sim.exe'
New-Item -ItemType Directory -Force (Split-Path $output) | Out-Null

& $cl /nologo /EHsc /std:c++17 $source "/Fe:$output"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Built $output"
if (-not $BuildOnly) {
    & $output
    exit $LASTEXITCODE
}
