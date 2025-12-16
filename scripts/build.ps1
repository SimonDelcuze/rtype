#Requires -Version 5.1
<#
.SYNOPSIS
    Build script for R-Type project on Windows.
.DESCRIPTION
    Configures and builds the R-Type project using CMake.
    Executables are automatically placed in the project root directory.
.PARAMETER Target
    Build target: all (default), client, server, or shared.
.EXAMPLE
    .\build.ps1
    .\build.ps1 -Target client
#>

param(
    [ValidateSet("all", "client", "server", "shared")]
    [string]$Target = "all"
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectRoot "build"

function Show-Usage {
    Write-Host "Usage: .\build.ps1 [-Target <all|client|server|shared>]"
    Write-Host "  all (default): build all targets"
    Write-Host "  client       : build only rtype_client target (generates r-type_client)"
    Write-Host "  server       : build only rtype_server target (generates r-type_server)"
    Write-Host "  shared       : build only rtype_shared_tests"
    exit 1
}

if (-not (Test-Path $BuildDir) -or -not (Test-Path (Join-Path $BuildDir "CMakeCache.txt"))) {
    Write-Host "[build.ps1] Configuring CMake in '$BuildDir'..."
    cmake -S $ProjectRoot -B $BuildDir -DBUILD_TESTS=ON
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed"
        exit $LASTEXITCODE
    }
}

$CmakeTargetArg = @()
switch ($Target) {
    "all" {
        Write-Host "[build.ps1] Building all targets..."
    }
    "client" {
        Write-Host "[build.ps1] Building client target..."
        $CmakeTargetArg = @("--target", "rtype_client")
    }
    "server" {
        Write-Host "[build.ps1] Building server target..."
        $CmakeTargetArg = @("--target", "rtype_server")
    }
    "shared" {
        Write-Host "[build.ps1] Building shared tests target..."
        $CmakeTargetArg = @("--target", "rtype_shared_tests")
    }
}

Write-Host "[build.ps1] Running cmake --build..."
cmake --build $BuildDir @CmakeTargetArg -j
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed"
    exit $LASTEXITCODE
}

Write-Host "[build.ps1] Build completed successfully!"
Write-Host "Executables are located in: $ProjectRoot"
