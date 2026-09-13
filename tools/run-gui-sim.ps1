param(
    [switch]$BuildOnly,
    [switch]$SmokeTest
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$deps = Join-Path $root '.deps'
$build = Join-Path $root '.sim-build'
$sdlVersion = '2.32.10'
$sdlArchive = Join-Path $deps "SDL2-devel-$sdlVersion-VC.zip"
$sdlRoot = Join-Path $deps "SDL2-$sdlVersion"
$sdlUrl = "https://github.com/libsdl-org/SDL/releases/download/release-$sdlVersion/SDL2-devel-$sdlVersion-VC.zip"

New-Item -ItemType Directory -Force $deps, $build | Out-Null
if (-not (Test-Path (Join-Path $sdlRoot 'include\SDL.h'))) {
    Write-Host "Downloading SDL2 $sdlVersion..."
    Invoke-WebRequest $sdlUrl -OutFile $sdlArchive
    Expand-Archive $sdlArchive -DestinationPath $deps -Force
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw 'Visual Studio Installer was not found.' }
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw 'Visual Studio C++ build tools were not found.' }
$vcVersion = (Get-Content (Join-Path $vsPath 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt') -Raw).Trim()
$vcTools = Join-Path $vsPath "VC\Tools\MSVC\$vcVersion"
$cl = Join-Path $vcTools 'bin\Hostx64\x64\cl.exe'

$sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
$sdkVersion = Get-ChildItem (Join-Path $sdkRoot 'Include') -Directory |
    Where-Object { Test-Path (Join-Path $_.FullName 'ucrt') } |
    Sort-Object { [version]$_.Name } -Descending |
    Select-Object -First 1 -ExpandProperty Name
if (-not $sdkVersion) { throw 'A Windows 10/11 SDK was not found.' }

$env:INCLUDE = @(
    (Join-Path $vcTools 'include'),
    (Join-Path $sdkRoot "Include\$sdkVersion\ucrt"),
    (Join-Path $sdkRoot "Include\$sdkVersion\um"),
    (Join-Path $sdkRoot "Include\$sdkVersion\shared"),
    (Join-Path $sdlRoot 'include')
) -join ';'
$env:LIB = @(
    (Join-Path $vcTools 'lib\x64'),
    (Join-Path $sdkRoot "Lib\$sdkVersion\ucrt\x64"),
    (Join-Path $sdkRoot "Lib\$sdkVersion\um\x64"),
    (Join-Path $sdlRoot 'lib\x64')
) -join ';'

$source = Join-Path $root 'simulator\main.cpp'
$exe = Join-Path $build 'tide-clock-sim.exe'
& $cl /nologo /EHsc /std:c++17 /O2 $source /link SDL2.lib shell32.lib "/out:$exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Copy-Item (Join-Path $sdlRoot 'lib\x64\SDL2.dll') $build -Force
Write-Host "Built $exe"

if (-not $BuildOnly) {
    if ($SmokeTest) { & $exe --smoke }
    else { & $exe }
    exit $LASTEXITCODE
}
