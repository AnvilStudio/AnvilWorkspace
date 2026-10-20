
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot

# Run the project with its local virtual environment when available.
$python = Join-Path $PSScriptRoot '.venv\Scripts\python.exe'
if (-not (Test-Path -LiteralPath $python)) {
	$command = Get-Command python -ErrorAction SilentlyContinue
	if ($command) {
		$python = $command.Source
	} else {
		$command = Get-Command py -ErrorAction SilentlyContinue
		if ($command) {
			& $command.Source -3 (Join-Path $PSScriptRoot 'forge_launcher.py') @args
			exit $LASTEXITCODE
		}
		Write-Error 'Python 3 was not found. Install Python and add it to PATH.'
		exit 1
	}
}

$entryPoint = Join-Path $PSScriptRoot 'forge_launcher.py'
if (-not (Test-Path -LiteralPath $entryPoint)) {
	Write-Error "Could not find the application entry point: $entryPoint"
	exit 1
}

& $python $entryPoint @args
exit $LASTEXITCODE
