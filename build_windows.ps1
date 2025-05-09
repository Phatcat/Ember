# Copyright (c) 2025 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Use vswhere.exe to locate the latest Visual Studio installation that includes the VC Tools.
$vsPath = & "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" `
          -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

# Build the full path to the VsDevCmd.bat from the installation path.
$vsDevCmd = Join-Path $vsPath "Common7\Tools\VsDevCmd.bat"

# Use the developer command prompt to set up the environment.
& "$vsDevCmd"

# Import the module
Import-Module AnyPackage

# List the currently registered providers
Write-Host "Registered package providers:"
Get-AnyPackageProvider

try {
    Write-Host "Attempting to install package 'Botan'..."
    Install-Package -Name Botan -Force
    Write-Host "Botan was installed (or already available) using default providers."
} catch {
    Write-Error "Failed to install Botan using default providers."
}

try {
    Write-Host "Attempting to install package 'Boost'..."
    Install-Package -Name Boost -Force
    Write-Host "Boost was installed (or already available) using default providers."
} catch {
    Write-Error "Failed to install Boost using default providers."
}

try {
    Write-Host "Attempting to install dependency 'mysql-client'..." -ForegroundColor Cyan
    Install-Package -Name "mysql-client" -Force -Verbose
    Write-Host "'mysql-client' installed successfully." -ForegroundColor Green
} catch {
    Write-Error "Failed to install dependency 'mysql-client' via default providers."
}

try {
    Write-Host "Attempting to install dependency 'pcre'..." -ForegroundColor Cyan
    Install-Package -Name "pcre" -Force -Verbose
    Write-Host "'pcre' installed successfully." -ForegroundColor Green
} catch {
    Write-Error "Failed to install dependency 'pcre' via default providers."
}

try {
    Write-Host "Attempting to install dependency 'flatbuffers'..." -ForegroundColor Cyan
    Install-Package -Name "flatbuffers" -Force -Verbose
    Write-Host "'flatbuffers' installed successfully." -ForegroundColor Green
} catch {
    Write-Error "Failed to install dependency 'flatbuffers' via default providers."
}

try {
    Write-Host "Attempting to install dependency 'mysql-connector-c++'..." -ForegroundColor Cyan
    Install-Package -Name "mysql-connector-c++" -Force -Verbose
    Write-Host "'mysql-connector-c++' installed successfully." -ForegroundColor Green
} catch {
    Write-Error "Failed to install dependency 'mysql-connector-c++' via default providers."
}

# List installed packages:
Write-Host "Installed packages:"
Get-Package

###############################
# Configure and Build Ember
###############################
Write-Host "=== Configuring project with CMake ==="

$buildDir            = "build"
$installDir          = ".\build\bin"
$generator           = "Visual Studio 17 2022"
$buildOptionalTools  = "-1"
$disableEmberThreads = "0"
$runtimeOption       = "MultiThreaded$<$<CONFIG:Debug>:Debug>"
$buildType           = "Debug"

cmake -S . -B $buildDir -G "$generator" `
      -DCMAKE_MSVC_RUNTIME_LIBRARY="$runtimeOption" `
      -DBUILD_OPT_TOOLS="$buildOptionalTools" `
      -DDISABLE_EMBER_THREADS="$disableEmberThreads" `
      -DCMAKE_INSTALL_PREFIX="$installDir"

Write-Host "Building and installing the project..."
cmake --build $buildDir --target install --config "$buildType"

###############################################
# Run the unit_tests for regression control
###############################################
Write-Host "=== Switching to installed directory and running tests ==="
Set-Location $installDir
if (Test-Path ".\unit_tests.exe") {
    .\unit_tests.exe
} else {
    Write-Host "Warning: Installed test executable not found."
}

Write-Host "=== Build, install, and test complete ==="
