$ErrorActionPreference='Stop'
$required=@('CMakeLists.txt','common/src/app.c','apps/radio/src/main.c','config/radio/stations.ini','scripts/build.sh','scripts/package.sh')
foreach($f in $required){if(!(Test-Path (Join-Path $PSScriptRoot "../$f"))){throw "Falta $f"}}
$ini=Get-Content -Raw (Join-Path $PSScriptRoot '../config/radio/stations.ini')
if($ini -notmatch '\[station\.1\]' -or $ini -notmatch 'codec=mp3'){throw 'stations.ini invalido'}
Write-Host 'OK: estructura y configuracion validadas.'