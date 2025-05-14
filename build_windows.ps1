# Copyright (c) 2025 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

$buildDir            = "build"
$installDir          = ".\build\bin"
$buildType           = "Debug"

$buildOptionalTools  = "-1"
$disableEmberThreads = "0"

# Determine the required VC Tools based on architecture.
if ($env:PROCESSOR_ARCHITECTURE -eq "AMD64") {
    $vcRequirement = "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
    $cpuTarget = "x64"
}
elseif ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") {
    $vcRequirement = "Microsoft.VisualStudio.Component.VC.Tools.ARM64"
    $cpuTarget = "arm64"
} 
else {
    Write-Error "Architecture not recognized: $env:PROCESSOR_ARCHITECTURE"
    exit 1
}

# Locate the appropriate Visual Studio installation.
$vsPath = & "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -products * -requires $vcRequirement -property installationPath

if (-not $vsPath) {
    Write-Error "Could not locate a Visual Studio installation with the required VC Tools: $vcRequirement"
    exit 1
}

# Build the full path to VsDevCmd.bat.
$vsDevCmd = Join-Path $vsPath "Common7\Tools\VsDevCmd.bat"

# Define a function to run a command with the freshly loaded VS environment.
function Invoke-WithDevEnv {
    param(
        [Parameter(Mandatory=$true)]
        [string]$CommandToRun
    )

    switch ($env:PROCESSOR_ARCHITECTURE) {
        "AMD64" { $archParam = "-arch=amd64" }
        "ARM64"  { $archParam = "-arch=arm64" }
        default  { Write-Warning "Unknown processor architecture: $env:PROCESSOR_ARCHITECTURE" }
    }

    $fullCommand = "call `"$vsDevCmd`" $archParam && $CommandToRun"
    Write-Host "Executing: $fullCommand"
    cmd /c $fullCommand
}

##############################################################
# WE NEED THE FOLLOWING DEPENDENCIES:
# Boost, Botan, Flatbuffers, mysql-connector-cpp, pcre, zlib
# We will install all of them from sources
##############################################################

#############################################################
# Install Boost from source (version 1.88.0) - Minimal Build
#############################################################
$boostTargetDir = "C:\boost"
if (-not (Test-Path $boostTargetDir)) {
    $zipFile = "boost_1_88_0.zip"
    $url = "https://archives.boost.io/release/1.88.0/source/boost_1_88_0.zip"

    Write-Host "Downloading Boost 1.88.0 from $url"
    if (Get-Command curl -ErrorAction SilentlyContinue) {
         Write-Host "Downloading using curl..."
         curl -L $url -o $zipFile
    }
    elseif (Get-Command wget -ErrorAction SilentlyContinue) {
         Write-Host "Downloading using wget..."
         wget $url -O $zipFile
    }
    else {
         Write-Host "Downloading using Invoke-WebRequest..."
         Invoke-WebRequest -Uri $url -OutFile $zipFile
    }

    Write-Host "Extracting Boost..."
    Expand-Archive -Path $zipFile -DestinationPath "boost_1_88_0" -Force
    Write-Host "Boost extracted..."
    # Use the expected source directory structure.
    $sourceBase = "boost_1_88_0\boost_1_88_0"
    if (-not (Test-Path $sourceBase)) {
         # Fallback if the folder structure is different.
         $sourceBase = "boost_1_88_0"
    }

    Push-Location $sourceBase

    Write-Host "Running Boost bootstrap..."
    & .\bootstrap.bat

    Write-Host "Building and installing minimal Boost to $boostTargetDir..."
    & .\b2 install --prefix="$boostTargetDir"  `
                   --with-headers `
                   --with-interprocess `
                   --with-program_options `
                   --with-system `
                   --with-uuid `
                   address-model=64

    Pop-Location
}
else {
    Write-Host "Boost is already installed at $boostTargetDir"
}

$env:CMAKE_PREFIX_PATH = "$boostTargetDir;$env:CMAKE_PREFIX_PATH"
Write-Host "CMAKE_PREFIX_PATH set to: $env:CMAKE_PREFIX_PATH"

#############################################################
# Botan Installation from Source (Version 3.8.1)
#############################################################
$botanTargetDir = "C:\botan"
if (-not (Test-Path $botanTargetDir)) {
    $zipFile = "botan-3.8.1.zip"
    $url = "https://github.com/randombit/botan/archive/refs/tags/3.8.1.zip"

    Write-Host "Downloading Botan 3.8.1 from $url"
    if (Get-Command curl -ErrorAction SilentlyContinue) {
        Write-Host "Downloading using curl..."
        curl -L $url -o $zipFile
    }
    elseif (Get-Command wget -ErrorAction SilentlyContinue) {
        Write-Host "Downloading using wget..."
        wget $url -O $zipFile
    }
    else {
        Write-Host "Downloading using Invoke-WebRequest..."
        Invoke-WebRequest -Uri $url -OutFile $zipFile
    }

    Write-Host "Extracting Botan..."
    Expand-Archive -Path $zipFile -DestinationPath "botan-3.8.1" -Force

    # Determine the source base folder.
    $sourceBase = "botan-3.8.1\botan-3.8.1"
    if (-not (Test-Path $sourceBase)) {
        $sourceBase = "botan-3.8.1"
    }

    Push-Location $sourceBase

    Write-Host "Configuring Botan build..."
    python configure.py --cc=msvc --os=windows --cpu="$env:PROCESSOR_ARCHITECTURE" --prefix="$botanTargetDir"

    Write-Host "Building Botan..."
    Invoke-WithDevEnv "nmake"

    Write-Host "Installing Botan..."
    Invoke-WithDevEnv "nmake install"

    Pop-Location
}
else {
    Write-Host "Botan is already installed at C:\botan"
}

$env:CMAKE_PREFIX_PATH = "$botanTargetDir;$env:CMAKE_PREFIX_PATH"
Write-Host "CMAKE_PREFIX_PATH set to: $env:CMAKE_PREFIX_PATH"

#############################################################
# Install FlatBuffers from source (version 25.2.10)
#############################################################
$flatbuffersTargetDir = "C:\flatbuffers"
if (-not (Test-Path $flatbuffersTargetDir)) {
    $zipFile = "flatbuffers-25.2.10.zip"
    $url = "https://github.com/google/flatbuffers/archive/refs/tags/v25.2.10.zip"

    Write-Host "Downloading FlatBuffers v25.2.10 from $url"
    if (Get-Command curl -ErrorAction SilentlyContinue) {
        Write-Host "Downloading using curl..."
        curl -L $url -o $zipFile
    }
    elseif (Get-Command wget -ErrorAction SilentlyContinue) {
        Write-Host "Downloading using wget..."
        wget $url -O $zipFile
    }
    else {
        Write-Host "Downloading using Invoke-WebRequest..."
        Invoke-WebRequest -Uri $url -OutFile $zipFile
    }

    Write-Host "Extracting FlatBuffers..."
    Expand-Archive -Path $zipFile -DestinationPath "flatbuffers-25.2.10" -Force

    # Use the expected source directory structure.
    $sourceBase = "flatbuffers-25.2.10\flatbuffers-25.2.10"
    if (-not (Test-Path $sourceBase)) {
        # Fallback if folder structure is different.
        $sourceBase = "flatbuffers-25.2.10"
    }

    New-Item -ItemType Directory -Path "$sourceBase\build" -Force | Out-Null

    Push-Location "$sourceBase\build"

    Write-Host "Configuring FlatBuffers build..."
    cmake .. -G "Visual Studio 17 2022" -A $cpuTarget `
      -DFLATBUFFERS_BUILD_TESTS=OFF `
      -DFLATBUFFERS_BUILD_FLATLIB=ON `
      -DCMAKE_INSTALL_PREFIX="$flatbuffersTargetDir"

    Write-Host "Building FlatBuffers..."
    cmake --build . --config $buildType

    Write-Host "Installing FlatBuffers to $flatbuffersTargetDir..."
    cmake --install . --config $buildType

    Pop-Location
} 
else {
    Write-Host "FlatBuffers is already installed at $flatbuffersTargetDir"
} 

$env:CMAKE_PREFIX_PATH = "$flatbuffersTargetDir;$env:CMAKE_PREFIX_PATH"
Write-Host "CMAKE_PREFIX_PATH set to: $env:CMAKE_PREFIX_PATH"

####################################################
# Install MySQL Connector/C++ 
# (Prebuilt for x86_64 / Source-compile for ARM64)
####################################################
$mysqlconcppTargetDir = "C:\mysql-connector-c++"
if (-not (Test-Path $mysqlconcppTargetDir)) {
    # Define the cache ZIP file path for MySQL Connector
    $zipFile = "mysql-connector.zip"

    Write-Host "Downloading MySQL Connector/C++ source code."
    $url = "https://github.com/mysql/mysql-connector-cpp/archive/refs/tags/9.3.0.zip"
    $connectorSubDir = "mysql-connector-cpp-9.3.0"

    Write-Host "Downloading MySQL Connector/C++ from $url"
    if (Get-Command curl -ErrorAction SilentlyContinue) {
        Write-Host "Downloading using curl..."
        curl -L $url -o $zipFile
    }
    elseif (Get-Command wget -ErrorAction SilentlyContinue) {
        Write-Host "Downloading using wget..."
        wget $url -O $zipFile
    }
    else {
        Write-Host "Downloading using Invoke-WebRequest..."
        Invoke-WebRequest -Uri $url -OutFile $zipFile
    }

    $extractDir = "mysql-connector-c++"
    New-Item -ItemType Directory -Path $extractDir | Out-Null

    Write-Host "Extracting MySQL Connector/C++..."
    Expand-Archive -Path $zipFile -DestinationPath $extractDir

    # Determine source base directory (if the ZIP extract creates a subfolder)
    $sourceBase = Join-Path $extractDir $connectorSubDir
    if (-not (Test-Path $sourceBase)) {
        $sourceBase = $extractDir
    }

    New-Item -ItemType Directory -Path "$sourceBase\build" -Force | Out-Null

    Push-Location "$sourceBase\build"

    Write-Host "Configuring with CMake..."
    cmake .. -G "Visual Studio 17 2022" -A $cpuTarget `
             -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" `
             -DBUILD_STATIC=ON `
             -DOPENSSL_USE_STATIC_LIBS=TRUE `
             -DWITH_JDBC=ON `
             -DWITH_MYSQL="C:\Program Files\MySQL\MySQL Server 8.0" `
             -DWITH_SSL="C:\Program Files\OpenSSL-Win64" `
             -DCMAKE_INSTALL_PREFIX="$mysqlconcppTargetDir"

    Write-Host "Building MySQL Connector/C++ Release..."
    cmake --build . --config Release

    Write-Host "Installing MySQL Connector/C++ to $mysqlconcppTargetDir..."
    cmake --install . --config Release

    # For some reason the config doesn't find the lib in the installed path...
    New-Item -ItemType Directory -Path "$mysqlconcppTargetDir\lib" -Force | Out-Null
    Copy-Item -Path (Join-Path "$mysqlconcppTargetDir\lib64\vs14" "*") `
              -Destination "$mysqlconcppTargetDir\lib" -Recurse -Force

    Write-Host "Building MySQL Connector/C++ Debug..."
    cmake --build . --config Debug

    Write-Host "Installing MySQL Connector/C++ to $mysqlconcppTargetDir\lib\debug..."
    cmake --install . --config Debug

    # For some reason the config doesn't find the lib in the installed path...
    New-Item -ItemType Directory -Path "$mysqlconcppTargetDir\lib" -Force | Out-Null
    Copy-Item -Path (Join-Path "$mysqlconcppTargetDir\lib64\debug\vs14" "*") `
              -Destination "$mysqlconcppTargetDir\lib\debug" -Recurse -Force

    Pop-Location

    Write-Host "MySQL Connector/C++ installed at: $mysqlconcppTargetDir"
} 
else {
    Write-Host "MySQL Connector/C++ is already installed at $mysqlconcppTargetDir"
}

$env:CMAKE_PREFIX_PATH = "$mysqlconcppTargetDir;$env:CMAKE_PREFIX_PATH"
Write-Host "CMAKE_PREFIX_PATH set to: $env:CMAKE_PREFIX_PATH"

#############################################################
# PCRE 8.45 Installation from Source (Non-autotools Build)
#############################################################
$pcreTargetDir = "C:\pcre"
if (-not (Test-Path $pcreTargetDir)) {
    $zipFile = "pcre-8.45.zip"
    $url = "https://mirror.ihost.md/exim/pcre/pcre-8.45.zip"

    Write-Host "Downloading PCRE 8.45 from $url"
    if (Get-Command curl -ErrorAction SilentlyContinue) {
         Write-Host "Downloading using curl..."
         curl -L $url -o $zipFile
    }
    elseif (Get-Command wget -ErrorAction SilentlyContinue) {
         Write-Host "Downloading using wget..."
         wget $url -O $zipFile
    }
    else {
         Write-Host "Downloading using Invoke-WebRequest..."
         Invoke-WebRequest -Uri $url -OutFile $zipFile
    }

    Write-Host "Extracting PCRE 8.45..."
    Expand-Archive -Path $zipFile -DestinationPath "pcre-8.45" -Force

    # Determine the source base folder.
    $sourceBase = "pcre-8.45\pcre-8.45"
    if (-not (Test-Path $sourceBase)) {
         $sourceBase = "pcre-8.45"
    }

    # Create a build directory within the source folder.
    New-Item -ItemType Directory -Path "$sourceBase\build" -Force | Out-Null

    Push-Location "$sourceBase\build"

    Write-Host "Configuring PCRE build..."
    cmake .. -G "Visual Studio 17 2022" -A $cpuTarget `
             -DCMAKE_INSTALL_PREFIX="$pcreTargetDir"

    # Release Build and Install
    Write-Host "Building PCRE in Release Configuration..."
    cmake --build . --config Release

    Write-Host "Installing PCRE (Release) to $pcreTargetDir..."
    cmake --install . --config Release

    # Debug Build and Install
    Write-Host "Building PCRE in Debug cConfiguration..."
    cmake --build . --config Debug

    $debugDir = "$pcreTargetDir\lib\debug"

    Write-Host "Installing PCRE (Debug) to $debugDir"
    cmake --install . --config Debug

    # Move the library from the lib folder to the newly-created debug folder and rename it
    New-Item -ItemType Directory -Path $debugDir -Force | Out-Null
    Write-Host "Created debug folder: $debugDir"
    Move-Item "$pcreTargetDir\lib\pcred.lib" "$debugDir\pcre.lib" -Force
    Write-Host "Moved 'pcred.lib' to '$debugDir\pcre.lib'"

    Pop-Location
}
else {
    Write-Host "PCRE 8.45 is already installed at $pcreTargetDir"
}

$env:CMAKE_PREFIX_PATH = "$pcreTargetDir;$env:CMAKE_PREFIX_PATH"
Write-Host "CMAKE_PREFIX_PATH set to: $env:CMAKE_PREFIX_PATH"

#############################################################
# ZLIB 1.3.1 Installation from Source (CMake-Based Build)
#############################################################
$zlibTargetDir = "C:\zlib"
if (-not (Test-Path $zlibTargetDir)) {
    $zipFile = "zlib131.zip"
    $url = "https://zlib.net/zlib131.zip"

    Write-Host "Downloading ZLIB 1.3.1 from $url"
    if (Get-Command curl -ErrorAction SilentlyContinue) {
         Write-Host "Downloading using curl..."
         curl -L $url -o $zipFile
    }
    elseif (Get-Command wget -ErrorAction SilentlyContinue) {
         Write-Host "Downloading using wget..."
         wget $url -O $zipFile
    }
    else {
         Write-Host "Downloading using Invoke-WebRequest..."
         Invoke-WebRequest -Uri $url -OutFile $zipFile
    }

    Write-Host "Extracting ZLIB 1.3.1..."
    Expand-Archive -Path $zipFile -DestinationPath "zlib-1.3.1" -Force

        # Determine the source base folder.
    $sourceBase = "zlib-1.3.1\zlib-1.3.1"
    if (-not (Test-Path $sourceBase)) {
         $sourceBase = "zlib-1.3.1"
    }
    
    # Create a build directory within the source folder.
    New-Item -ItemType Directory -Path "$sourceBase\build" -Force | Out-Null

    Push-Location "$sourceBase\build"

    Write-Host "Configuring ZLIB build using CMake..."
    cmake .. -G "Visual Studio 17 2022" -A $cpuTarget `
             -DCMAKE_INSTALL_PREFIX="$zlibTargetDir"

    Write-Host "Building ZLIB..."
    cmake --build . --config $buildType

    Write-Host "Installing ZLIB to $zlibTargetDir"
    cmake --install . --config $buildType

    Pop-Location
} 
else {
    Write-Host "ZLIB 1.3.1 is already installed at $zlibTargetDir"
}

$env:CMAKE_PREFIX_PATH = "$zlibTargetDir;$env:CMAKE_PREFIX_PATH"
Write-Host "CMAKE_PREFIX_PATH set to: $env:CMAKE_PREFIX_PATH"

####################################################
# Configure and Build Ember
####################################################
Write-Host "=== Configuring project with CMake ==="

cmake -S . -B $buildDir -G "Visual Studio 17 2022" `
      -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" `
      -DBUILD_OPT_TOOLS="$buildOptionalTools" `
      -DDISABLE_EMBER_THREADS="$disableEmberThreads" `
      -DCMAKE_INSTALL_PREFIX="$installDir"

Write-Host "Building and installing the project..."
cmake --build $buildDir --target install --config "$buildType"

####################################################
# Run the unit_tests for regression control
####################################################
#Write-Host "=== Switching to installed directory and running tests ==="
#Set-Location $installDir
#if (Test-Path ".\unit_tests.exe") {
#    .\unit_tests.exe
#} 
#else {
#    Write-Host "Warning: Installed test executable not found."
#}
#
#Write-Host "=== Build, install, and test complete ==="
