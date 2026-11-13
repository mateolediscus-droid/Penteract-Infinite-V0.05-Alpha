param([switch]$Rebuild)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio 2022 C++ Build Tools (v143) and a Windows SDK.' }
$msbuild = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\**\Bin\amd64\MSBuild.exe' | Select-Object -First 1
if (!$msbuild) { throw 'MSBuild / Visual C++ tools not found.' }
$env:TEMP = Join-Path $root 'build-temp'
$env:TMP = $env:TEMP
$env:_CL_ = '/MP1'
New-Item -ItemType Directory -Force -Path $env:TEMP | Out-Null
Push-Location $root
try {
    $target = if ($Rebuild) { '/t:Rebuild' } else { '/t:Build' }
    & $msbuild '5D-Miner.sln' $target /p:Configuration=Release /p:Platform=x64 /m:1 /nr:false /v:minimal
    if ($LASTEXITCODE -ne 0) { throw 'Game build failed.' }
    Copy-Item -LiteralPath 'x64\Release\Penteract Placer.exe' -Destination 'Pentaract Infinite.exe' -Force
    Get-ChildItem -LiteralPath 'x64\Release' -Filter '*.dll' -File | Copy-Item -Destination $root -Force
    Write-Output 'Ready: Pentaract Infinite.exe (uses the assets and TextureProfiles in this folder).'
} finally { Pop-Location }
