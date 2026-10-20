param(
    [switch]$Build,
    [ValidateSet("DebugG", "Debug", "Release")]
    [string]$Configuration = "Debug"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# Keep this pinned version in sync with PYTHON_VERSION in premake5.lua.
$PythonVersion = "3.13.11"
$Root = $PSScriptRoot
$PythonRoot = Join-Path $Root "Dependencies\Python"
$RuntimeDir = Join-Path $PythonRoot "runtime"
$SdkDir = Join-Path $PythonRoot "sdk"
$CacheDir = Join-Path $PythonRoot ".cache"
$VersionFile = Join-Path $PythonRoot "version.txt"

$RuntimeUrl = "https://www.python.org/ftp/python/$PythonVersion/python-$PythonVersion-embed-amd64.zip"
$SdkUrl = "https://api.nuget.org/v3-flatcontainer/python/$PythonVersion/python.$PythonVersion.nupkg"

function Install-Archive {
    param([string]$Url, [string]$Destination, [string]$ArchiveName)

    $DownloadPath = Join-Path $CacheDir $ArchiveName
    $TempPath = "$Destination.tmp"

    Remove-Item -LiteralPath $TempPath -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $DownloadPath -Force -ErrorAction SilentlyContinue

    try {
        Write-Host "Downloading $Url"
        Invoke-WebRequest -Uri $Url -OutFile $DownloadPath -UseBasicParsing

        # Expand-Archive only accepts .zip extension, including for NuGet packages.
        New-Item -ItemType Directory -Force -Path $TempPath | Out-Null
        Expand-Archive -LiteralPath $DownloadPath -DestinationPath $TempPath -Force

        if (Test-Path -LiteralPath $Destination) {
            Remove-Item -LiteralPath $Destination -Recurse -Force
        }
        Move-Item -LiteralPath $TempPath -Destination $Destination
    }
    finally {
        Remove-Item -LiteralPath $DownloadPath -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $TempPath -Recurse -Force -ErrorAction SilentlyContinue
    }
}

if (-not [Environment]::Is64BitOperatingSystem) {
    throw "Anvil currently requires Windows x64."
}

New-Item -ItemType Directory -Force -Path $CacheDir | Out-Null

$CurrentVersion = if (Test-Path $VersionFile) { (Get-Content $VersionFile -Raw).Trim() } else { "" }
if ($CurrentVersion -ne $PythonVersion) {
    Write-Host "Python dependency version changed; reinstalling..."
    Remove-Item -LiteralPath $RuntimeDir, $SdkDir -Recurse -Force -ErrorAction SilentlyContinue
}

$RuntimeFiles = @("python313.dll", "python313.zip", "python313._pth")
$SdkFiles = @("tools\include\Python.h", "tools\libs\python313.lib")

$RuntimeReady = (Test-Path $RuntimeDir)
foreach ($File in $RuntimeFiles) {
    if (-not (Test-Path (Join-Path $RuntimeDir $File))) { $RuntimeReady = $false }
}
if (-not $RuntimeReady) {
    Install-Archive -Url $RuntimeUrl -Destination $RuntimeDir -ArchiveName "python-runtime.zip"
}

$SdkReady = (Test-Path $SdkDir)
foreach ($File in $SdkFiles) {
    if (-not (Test-Path (Join-Path $SdkDir $File))) { $SdkReady = $false }
}
if (-not $SdkReady) {
    # The NuGet python package includes development headers and import libraries.
    Install-Archive -Url $SdkUrl -Destination $SdkDir -ArchiveName "python-sdk.zip"
}

Set-Content -LiteralPath $VersionFile -Value $PythonVersion
Write-Host "Python $PythonVersion runtime and SDK ready: $PythonRoot"

$Premake = Get-Command "premake5" -ErrorAction SilentlyContinue
if (-not $Premake) {
    $LocalPremake = Join-Path $Root "premake5.exe"
    if (Test-Path $LocalPremake) {
        $PremakePath = $LocalPremake
    } else {
        Write-Host "Dependencies are ready. Install premake5 or place premake5.exe in the repository root to generate the solution."
        exit 0
    }
} else {
    $PremakePath = $Premake.Source
}

Push-Location $Root
try {
    & $PremakePath vs2026
    if ($LASTEXITCODE -ne 0) { throw "Premake project generation failed." }

    if ($Build) {
        $MSBuild = Get-Command "MSBuild.exe" -ErrorAction SilentlyContinue
        if ($MSBuild) {
            $MSBuildPath = $MSBuild.Source
        } else {
            $VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
            if (-not (Test-Path $VsWhere)) { throw "MSBuild not found. Run from a VS Developer PowerShell, or use without -Build." }
            $MSBuildPath = & $VsWhere -latest -products '*' -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
            if (-not $MSBuildPath) { throw "MSBuild not found." }
        }
        & $MSBuildPath "AnvilWorkspace.sln" "/m" "/p:Configuration=$Configuration" "/p:Platform=x64"
        if ($LASTEXITCODE -ne 0) { throw "Anvil build failed." }
    }
}
finally {
    Pop-Location
}
