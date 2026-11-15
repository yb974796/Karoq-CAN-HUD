[CmdletBinding()]
param([string]$Project = 'D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_Integrated')
$ErrorActionPreference='Stop'
$main = Join-Path $Project 'main'
function AssertFile([string]$relative) {
    $p=Join-Path $main $relative
    if(-not(Test-Path -LiteralPath $p)) { throw "MISSING: $relative" }
    Write-Host ('PASS: '+$relative)
}
foreach($p in @('core\main.c','id\can\karoq_can_sniffer.c',
'id\decoders\karoq_can_speed.c','id\decoders\karoq_can_acc_probe.c',
'ui\bridge\karoq_hud_speed_bridge.c','ui\guard\karoq_hud_speed_guard.c',
'ui\legacy\karoq_hud.c','ui\legacy\karoq_hud_assets.c','CMakeLists.txt')) {
 AssertFile $p
}
$core=[IO.File]::ReadAllText((Join-Path $main 'core\main.c'))
if($core.Contains('karoq_hud_v08_ui_attach();')) { throw 'Obsolete V0.8 overlay still active' }
if(([regex]::Matches($core,'karoq_hud_speed_bridge_attach\(\);')).Count -ne 1) { throw 'Bridge attach must appear exactly once' }
if(([regex]::Matches($core,'karoq_can_sniffer_start\(\)')).Count -ne 1) { throw 'TWAI start must appear exactly once' }
$hud=[IO.File]::ReadAllText((Join-Path $main 'ui\legacy\karoq_hud.c'))
if(-not $hud.Contains('#include "karoq_hud_speed_intercept.h"')) { throw 'Demo speed guard missing in HUD' }
$cmakeContent=[IO.File]::ReadAllText((Join-Path $main 'CMakeLists.txt'))
if($cmakeContent -match 'file\s*\(GLOB[^\)]*\bCONFIGURE_DEPENDS\b') { throw 'CMake uses CONFIGURE_DEPENDS; ESP-IDF script-mode fails' }
foreach($file in @('core/main.c','id/can/karoq_can_sniffer.c','id/decoders/karoq_can_speed.c',
 'id/decoders/karoq_can_acc_probe.c','ui/bridge/karoq_hud_speed_bridge.c',
 'ui/guard/karoq_hud_speed_guard.c')) {
 if(-not $cmakeContent.Contains(('"'+$file+'"'))) { throw ('CMake missing source: '+$file) }
}
Write-Host 'PASS: CMake script-mode compatible (no CONFIGURE_DEPENDS)' -ForegroundColor Green
Write-Host 'PASS: original UI + exactly one CAN and speed bridge; V0.8 overlay absent' -ForegroundColor Green
