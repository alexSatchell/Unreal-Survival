param(
	[string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8'
)

$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'Survival.uproject'
$buildScript = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'

if (-not (Test-Path -LiteralPath $buildScript))
{
	throw "Unreal build script not found: $buildScript. Supply -EngineRoot with the Unreal 5.8 installation directory."
}

# Keep accelerator files within this checkout and avoid result-cache stalls.
$previousUbaRoot = $env:UBA_ROOT
try
{
	$env:UBA_ROOT = Join-Path $projectRoot 'Intermediate\CodexUBA'
	& $buildScript SurvivalEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoUBA -nocache
	$buildExitCode = $LASTEXITCODE
}
finally
{
	$env:UBA_ROOT = $previousUbaRoot
}

exit $buildExitCode
