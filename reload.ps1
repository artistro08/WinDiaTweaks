# Rebuild, restart Dia, and re-attach the tweak engine. Needed whenever the DLL changes,
# because a loaded DLL cannot be replaced in a running process.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

Get-Process Dia -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 2

cmd /d /c "`"$root\build.cmd`""
if ($LASTEXITCODE -ne 0) { throw "build failed" }

if (Test-Path bin\tap.log) { Move-Item -Force bin\tap.log bin\tap.log.prev }
cmd /c "bin\diatweaks.exe restart 2>&1"
Start-Sleep -Seconds 2
Get-Content bin\tap.log -ErrorAction SilentlyContinue
