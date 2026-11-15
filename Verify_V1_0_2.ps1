[CmdletBinding()]
param([string]$Project = 'D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_2')
$ErrorActionPreference = 'Stop'
$py = Get-Command python -ErrorAction Stop
& $py.Source (Join-Path $PSScriptRoot 'tools\verify_v102.py') --project $Project
if ($LASTEXITCODE -ne 0) { throw "V1.0.2 verification failed, exit $LASTEXITCODE" }
