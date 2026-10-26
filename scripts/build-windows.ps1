# Build the Windows x64 Release player.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts\build-windows.ps1 `
#       -RomDir C:\path\to\roms-src -SdlDir C:\path\to\SDL2-2.30.9\cmake
param(
    [string]$RomDir = (Join-Path $PSScriptRoot '..\roms-src'),
    [string]$SdlDir = '',
    [string]$BuildDir = 'build-windows'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RomDir = (Resolve-Path $RomDir).Path
$build = Join-Path $root $BuildDir

# Pick the installed Visual Studio generator (2026 if present, else 2022).
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vsVer = & $vswhere -latest -products * -property installationVersion
$major = ([regex]::Match("$vsVer", '^(\d+)')).Groups[1].Value
$gen = if ($major -eq '18') { 'Visual Studio 18 2026' } else { 'Visual Studio 17 2022' }

$cfg = @('-S', $root, '-B', $build, '-G', $gen, '-A', 'x64', "-DMSLUG3_ROM_DIR=$RomDir")
if ($SdlDir) { $cfg += "-DSDL2_DIR=$SdlDir" }
& cmake @cfg
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
& cmake --build $build --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw 'build failed' }
Write-Host "built $build\Release\Metal_Slug_3.exe" -ForegroundColor Green
