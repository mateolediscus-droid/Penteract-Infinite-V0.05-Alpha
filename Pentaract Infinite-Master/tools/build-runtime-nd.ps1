param(
    [Parameter(Mandatory=$true)][ValidateRange(34,1073741791)][int]$Dimension,
    [Parameter(Mandatory=$true)][string]$Result
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root "nd-cache\$Dimension"
$lock = $null
function Report-Phase([string]$phase) {
    $progress = $Result + '.progress.json'
    @{phase=$phase} | ConvertTo-Json | Set-Content -LiteralPath ($progress + '.tmp') -Encoding UTF8
    Move-Item -LiteralPath ($progress + '.tmp') -Destination $progress -Force
}
function Fingerprint {
    $files = @(
        Get-ChildItem -LiteralPath $root -File | Where-Object { $_.Extension -in '.cpp','.h','.inl','.inc','.vcxproj' }
        Get-ChildItem -LiteralPath (Join-Path $root 'experiments') -File -Recurse | Where-Object { $_.Extension -in '.h','.inl','.cpp','.inc' }
        Get-Item -LiteralPath $PSCommandPath
        if (Test-Path -LiteralPath (Join-Path $root '5D-Miner\x64\Release')) {
            Get-ChildItem -LiteralPath (Join-Path $root '5D-Miner\x64\Release') -Filter '*.obj' -File -Recurse
        }
        Get-ChildItem -LiteralPath (Join-Path $root 'x64\Release') -Filter '*.dll' -File
    ) | Sort-Object FullName
    $metadata = ($files | ForEach-Object { "$($_.FullName)|$($_.Length)|$($_.LastWriteTimeUtc.Ticks)" }) -join "`n"
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($metadata)))).Replace('-','') }
    finally { $sha.Dispose() }
}
try {
    Report-Phase 'Checking compiler cache...'
    New-Item -ItemType Directory -Force -Path $cache | Out-Null
    $log = Join-Path $cache 'build.log'
    $exe = Join-Path $cache "bin\ND-$Dimension.exe"
    $stamp = Join-Path $cache 'fingerprint.txt'
    # One writer across dimensions: all variants reuse the base Release objects.
    $lockPath = Join-Path $root 'nd-cache\build.lock'
    Report-Phase 'Waiting for compiler...'
    while (!$lock) {
        try { $lock = [IO.File]::Open($lockPath,'OpenOrCreate','ReadWrite','None') }
        catch [IO.IOException] { Start-Sleep -Milliseconds 250 }
    }
    $fingerprint = Fingerprint
    $missingDll = @(Get-ChildItem -LiteralPath (Join-Path $root 'x64\Release') -Filter '*.dll' -File | Where-Object { !(Test-Path -LiteralPath (Join-Path (Split-Path -Parent $exe) $_.Name)) }).Count -gt 0
    $rebuild = $missingDll -or !(Test-Path -LiteralPath $exe) -or !(Test-Path -LiteralPath $stamp) -or (Get-Content -LiteralPath $stamp -Raw).Trim() -ne $fingerprint
    if ($rebuild) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio C++ tools are missing (vswhere.exe).' }
        $msbuild = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\**\Bin\amd64\MSBuild.exe' | Select-Object -First 1
        if (!$msbuild) { throw 'Visual Studio C++ compiler / MSBuild not found.' }
        $env:TEMP = Join-Path $cache 'temp'
        $env:TMP = $env:TEMP
        $env:_CL_ = '/MP1'
        New-Item -ItemType Directory -Force -Path $env:TEMP | Out-Null
        Push-Location $root
        try {
            Report-Phase '1/3 - Updating engine objects...'
            # The running game's executable need not be linked again for a new dimension.
            & $msbuild '5D-Miner.vcxproj' '/t:ClCompile;ResourceCompile' /p:Configuration=Release /p:Platform=x64 /m:1 /nr:false /v:minimal "/flp:logfile=$log;verbosity=normal"
            if ($LASTEXITCODE -ne 0) { throw "Base build failed. Log: $log" }
            Report-Phase "2/3 - Compiling and linking ${Dimension}D..."
            & $msbuild 'RuntimeWorldND.vcxproj' /p:Configuration=Release /p:Platform=x64 "/p:NdDimension=$Dimension" /m:1 /nr:false /v:minimal "/flp:logfile=$log;verbosity=normal;append"
            if ($LASTEXITCODE -ne 0) { throw "${Dimension}D build failed. Log: $log" }
        } finally { Pop-Location }
        if (!(Test-Path -LiteralPath $exe)) { throw "Build produced no executable. Log: $log" }
        Get-ChildItem -LiteralPath (Join-Path $root 'x64\Release') -Filter '*.dll' -File | Copy-Item -Destination (Split-Path -Parent $exe) -Force
        Report-Phase '3/3 - Checking compiled engine...'
        $probe = Start-Process -FilePath $exe -ArgumentList @('--nd-probe',"$Dimension") -WindowStyle Hidden -Wait -PassThru
        if ($probe.ExitCode -ne 0) { throw "Compiled engine registry / DLL check failed ($($probe.ExitCode)). Log: $log" }
        Fingerprint | Set-Content -LiteralPath $stamp -Encoding ASCII
    }
    Report-Phase 'Ready - launching world...'
    @{success=$true;executable=$exe;dimension=$Dimension;cacheHit=(!$rebuild);log=$log} | ConvertTo-Json | Set-Content -LiteralPath $Result -Encoding UTF8
} catch {
    @{success=$false;error=$_.Exception.Message;dimension=$Dimension} | ConvertTo-Json | Set-Content -LiteralPath $Result -Encoding UTF8
    exit 1
} finally {
    if ($lock) { $lock.Dispose() }
}
