# Copyright (c) 2025 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

#############################################################
# Install dependencies through conan
# Use pip for getting conan if it's not already available
#############################################################
Write-Host "Installing dependencies through Conan..."
if (-not (Get-Command conan.exe -ErrorAction SilentlyContinue)) {
    Write-Host "Conan not found. Installing Conan via pip..."
    pip install conan --user
    $ConanScripts = "$env:APPDATA\Python\Python39\Scripts"
    if (-not ($env:PATH -like "*$ConanScripts*")) {
        Write-Host "Adding $ConanScripts to PATH"
        $env:PATH += ";$ConanScripts"
    }
} else {
    Write-Host "Conan is already installed."
}

# Detect and patch the default profile for Debug and C++23
conan profile detect
$profilePath = (& conan profile path default).Trim()
(Get-Content $profilePath) `
    -replace '^(build_type=).*$', 'build_type=Debug' `
    -replace '^(compiler\.cppstd=).*$', 'compiler.cppstd=23' `
    | Set-Content $profilePath -Force

$conanCmd = "conan install"

# Dependencies to install via Conan.
$conanCmd += " --requires boost/1.87.0"
$conanCmd += " --requires botan/3.6.1"
$conanCmd += " --requires pcre/8.45"

$buildDir = "build"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

$conanCmd += " -of $buildDir --build missing -g CMakeToolchain -g CMakeDeps --profile default"
Write-Host "Running Conan install..."
Invoke-Expression $conanCmd

# --- Patch Conan configuration ---
# Define the top-level CMakeLists.txt path (assumed to be in the project root)
$cmakeFile = "CMakeLists.txt"

# Read the entire file into a single string (assuming the file exists)
$cmakeContent = Get-Content $cmakeFile -Raw

# --- Patch the Botan block ---
# We match from "if(TARGET Botan::Botan-static)" and ends with the first "endif()"
$botanRegex = '(?ms)if\s*\(TARGET\s+Botan::Botan-static\).*?endif\(\)'
$newBotanBlock = @"
if(TARGET botan::botan-static)
  set(BOTAN_LIBRARY botan::botan-static)
  message(STATUS "Using Botan static library")
elseif(TARGET botan::botan)
  set(BOTAN_LIBRARY botan::botan)
  message(STATUS "Using Botan shared library")
else()
  message(FATAL_ERROR "No valid Botan target found")
endif()
"@
$cmakeContent = [regex]::Replace($cmakeContent, $botanRegex, $newBotanBlock, 
                                 [System.Text.RegularExpressions.RegexOptions]::Singleline)

# --- Patch the PCRE block ---
# Find the PCRE find_package line and then append a line setting PCRE_LIBRARY to pcre::pcre.
$pcrePattern = "(find_package\(PCRE\s+8\.39\s+REQUIRED\))"
$pcreReplacement = '$1' + "`nset(PCRE_LIBRARY pcre::pcre)"
$cmakeContent = [regex]::Replace($cmakeContent, $pcrePattern, $pcreReplacement)

# Write the modified content back to the CMakeLists.txt
Set-Content -Path $cmakeFile -Value $cmakeContent
Write-Host "Updated top-level CMakeLists.txt successfully."

#############################################################
# Install FlatBuffers from source (version 2.0.8)
#############################################################
$flatbuffersTargetDir = "C:\flatbuffers"
if (-not (Test-Path $flatbuffersTargetDir)) {
    # Define a local cache directory for downloads (relative to this script)
    $cacheDir = Join-Path $PSScriptRoot "dependencies"
    if (-not (Test-Path $cacheDir)) {
        New-Item -ItemType Directory -Path $cacheDir | Out-Null
    }

    # Define the cache ZIP file path for FlatBuffers
    $CACHE_ZIP = Join-Path $cacheDir "flatbuffers-2.0.8.zip"

    # Download the archive if not cached
    if (Test-Path $CACHE_ZIP) {
        Write-Host "Cached FlatBuffers zip found."
    } else {
        # Use FlatBuffers version 2.0.8; if that tag is unavailable, you may choose a newer one.
        $url = "https://github.com/google/flatbuffers/archive/refs/tags/v2.0.8.zip"
        Write-Host "Downloading FlatBuffers v2.0.8 from $url"
        if (Get-Command curl -ErrorAction SilentlyContinue) {
            Write-Host "Downloading using curl..."
            curl -L $url -o $CACHE_ZIP
        } elseif (Get-Command wget -ErrorAction SilentlyContinue) {
            Write-Host "Downloading using wget..."
            wget $url -O $CACHE_ZIP
        } else {
            Write-Host "Downloading using Invoke-WebRequest..."
            Invoke-WebRequest -Uri $url -OutFile $CACHE_ZIP
        }
    }

    # Set extraction directory for FlatBuffers (within the cache folder)
    $extractDir = Join-Path $cacheDir "flatbuffers"
    if (Test-Path $extractDir) {
        Remove-Item $extractDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $extractDir | Out-Null

    Write-Host "Extracting FlatBuffers..."
    Expand-Archive -Path $CACHE_ZIP -DestinationPath $extractDir

    # The extracted folder is typically named "flatbuffers-v2.0.8"
    $sourceBase = Join-Path $extractDir "flatbuffers-v2.0.8"
    if (-not (Test-Path $sourceBase)) {
        # Fallback if the folder name deviates
        $sourceBase = $extractDir
    }

    # Create a build directory inside the source directory
    $buildDir = Join-Path $sourceBase "build"
    if (Test-Path $buildDir) {
        Remove-Item $buildDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $buildDir | Out-Null

    Write-Host "Configuring FlatBuffers..."
    cmake -S $sourceBase -B $buildDir -G "Visual Studio 17 2022" -DCMAKE_INSTALL_PREFIX=$flatbuffersTargetDir -DCMAKE_BUILD_TYPE=Release

    Write-Host "Building FlatBuffers..."
    cmake --build $buildDir --config Release

    Write-Host "Installing FlatBuffers to $flatbuffersTargetDir..."
    cmake --build $buildDir --target install --config Release
} else {
    Write-Host "FlatBuffers is already installed at $flatbuffersTargetDir"
}

# Set the environment path so that CMake finds the self-compiled FlatBuffers installation.
$env:CMAKE_PREFIX_PATH = "$flatbuffersTargetDir;$env:CMAKE_PREFIX_PATH"
Write-Host "CMAKE_PREFIX_PATH set to: $env:CMAKE_PREFIX_PATH"

#############################################################
# Install MySQL Connector/C++ (Prebuilt for x86_64 / Source for ARM64)
#############################################################
if (-not (Test-Path "C:\mysql-connector-c++\")) {
    # Define a local cache directory for downloads (relative to this script)
    $cacheDir = Join-Path $PSScriptRoot "dependencies"
    if (-not (Test-Path $cacheDir)) {
        New-Item -ItemType Directory -Path $cacheDir | Out-Null
    }

    # Define the cache ZIP file path for MySQL Connector
    $CACHE_ZIP = Join-Path $cacheDir "mysql-connector.zip"

    # Download the archive if not cached
    if (Test-Path $CACHE_ZIP) {
        Write-Host "Cached MySQL Connector zip found."
    } else {
        # Determine architecture and set download parameters accordingly
        if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") {
            Write-Host "ARM64 architecture detected. Downloading MySQL Connector/C++ source code."
            $url = "https://github.com/mysql/mysql-connector-cpp/archive/refs/tags/9.3.0.zip"
            $connectorSubDir = "mysql-connector-c++-9.3.0"
        } else {
            Write-Host "x86_64 architecture detected. Using prebuilt MySQL Connector/C++ binaries."
            $url = "https://dev.mysql.com/get/Downloads/Connector-C++/mysql-connector-c++-9.3.0-winx64-debug.zip"
            $connectorSubDir = "mysql-connector-c++-9.3.0-winx64-debug"
        }
        Write-Host "Downloading MySQL Connector/C++ from $url"

        if (Get-Command curl -ErrorAction SilentlyContinue) {
            Write-Host "Downloading using curl..."
            curl -L $url -o $CACHE_ZIP
        } elseif (Get-Command wget -ErrorAction SilentlyContinue) {
            Write-Host "Downloading using wget..."
            wget $url -O $CACHE_ZIP
        } else {
            Write-Host "Downloading using Invoke-WebRequest..."
            Invoke-WebRequest -Uri $url -OutFile $CACHE_ZIP
        }
    }

    # Set extraction directory for the connector (within the cache folder)
    $extractDir = Join-Path $cacheDir "mysql-connector-c++"
    if (Test-Path $extractDir) {
        Remove-Item $extractDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $extractDir | Out-Null

    Write-Host "Extracting MySQL Connector/C++..."
    Expand-Archive -Path $CACHE_ZIP -DestinationPath $extractDir

    # Determine source base directory (if the ZIP extract creates a subfolder)
    $sourceBase = Join-Path $extractDir $connectorSubDir
    if (-not (Test-Path $sourceBase)) {
        $sourceBase = $extractDir
    }

    if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") {
        ######################################################################
        # For ARM64: Downloaded Source – Build Required
        ######################################################################
        Write-Host "ARM64 architecture: MySQL Connector/C++ source downloaded."
        Write-Host "Source code is available at: $sourceBase"
        Write-Host "You must now build the connector from source for ARM64."
        Write-Host "For example, use CMake along with your preferred build configuration."
    } else {
        ######################################################################
        # For x86_64: Install Prebuilt Libraries
        ######################################################################
        # Define the base installation directory (de facto standard for Connector/C++ on Windows)
        $targetDir = "C:\mysql-connector-c++"

        if (-not (Test-Path $targetDir)) {
            New-Item -ItemType Directory -Path $targetDir | Out-Null
        }

        Write-Host "Installing MySQL Connector/C++ (prebuilt x86_64) to $targetDir..."

        # Set the expected extracted folder name from the ZIP archive (as provided by Oracle)
        $sourceBase = Join-Path $extractDir "mysql-connector-c++-9.3.0-winx64"

        # Instead of selecting only a few subdirectories, copy the entire contents to preserve the layout.
        Copy-Item -Path (Join-Path $sourceBase "*") -Destination $targetDir -Recurse -Force

        Write-Host "MySQL Connector/C++ installed at: $targetDir"
    }
} else {
    Write-Host "MySQL Connector/C++ is already installed at C:\mysql-connector-c++\"
}

# Set the env path so cmake knows where mysql-concpp is installed
$env:CMAKE_PREFIX_PATH = "C:\mysql-connector-c++"

###############################
# Configure and Build Ember
###############################
Write-Host "=== Configuring project with CMake ==="

$buildDir            = "build"
$installDir          = ".\build\bin"
$generator           = "Visual Studio 17 2022"
$toolchainFile       = "$buildDir\conan_toolchain.cmake"
$buildOptionalTools  = "-1"
$disableEmberThreads = "0"
$runtimeOption       = "MultiThreaded$<$<CONFIG:Debug>:Debug>"
$buildType           = "Debug"

cmake -S . -B $buildDir -G "$generator" `
      -DCMAKE_TOOLCHAIN_FILE="$toolchainFile" `
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
