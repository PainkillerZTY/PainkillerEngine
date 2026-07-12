# PAINKILLER Engine - Dependencies Setup Script
# Downloads and builds third-party libraries

$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$ThirdParty = Join-Path $Root "third_party"
$BuildDir = Join-Path $Root "build_deps"

# Ensure third_party directory
New-Item -ItemType Directory -Force -Path $ThirdParty | Out-Null
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

Write-Host "=== PAINKILLER Engine Dependency Setup ===" -ForegroundColor Cyan

# ?? 1. GLM (header-only math library) ??
$GLM_Version = "1.0.0"
$GLM_Dir = Join-Path $ThirdParty "glm"
New-Item -ItemType Directory -Force -Path $GLM_Dir | Out-Null

$GLM_HeaderDir = Join-Path $GLM_Dir "glm"
if (-not (Test-Path (Join-Path $GLM_HeaderDir "glm.hpp"))) {
    Write-Host "[1/3] Downloading GLM $GLM_Version..." -ForegroundColor Yellow
    $GLM_Zip = Join-Path $BuildDir "glm.zip"
    try {
        Invoke-WebRequest -Uri "https://github.com/g-truc/glm/releases/download/$GLM_Version/glm-$GLM_Version-light.zip" -OutFile $GLM_Zip -ErrorAction Stop
        Expand-Archive -Path $GLM_Zip -DestinationPath $BuildDir -Force
        # Copy glm/glm -> third_party/glm/glm
        Copy-Item -Path (Join-Path $BuildDir "glm\glm") -Destination $GLM_HeaderDir -Recurse -Force
        Write-Host "  GLM installed." -ForegroundColor Green
    } catch {
        Write-Host "  Failed to download GLM from release. Trying GitHub archive..." -ForegroundColor Yellow
        try {
            Remove-Item $GLM_Zip -ErrorAction SilentlyContinue
            Invoke-WebRequest -Uri "https://github.com/g-truc/glm/archive/refs/tags/$GLM_Version.zip" -OutFile $GLM_Zip -ErrorAction Stop
            Expand-Archive -Path $GLM_Zip -DestinationPath $BuildDir -Force
            Copy-Item -Path (Join-Path $BuildDir "glm-1.0.0\glm") -Destination $GLM_HeaderDir -Recurse -Force
            Write-Host "  GLM installed." -ForegroundColor Green
        } catch {
            Write-Host "  [WARN] Could not download GLM. Manual download required." -ForegroundColor Red
        }
    }
} else {
    Write-Host "[1/3] GLM already present." -ForegroundColor Green
}

# ?? 2. stb_image.h ??
$STB_Dir = Join-Path $ThirdParty "stb"
New-Item -ItemType Directory -Force -Path $STB_Dir | Out-Null
$STB_Path = Join-Path $STB_Dir "stb_image.h"
if (-not (Test-Path $STB_Path)) {
    Write-Host "[2/3] Downloading stb_image.h..." -ForegroundColor Yellow
    try {
        Invoke-WebRequest -Uri "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" -OutFile $STB_Path -ErrorAction Stop
        Write-Host "  stb_image.h installed." -ForegroundColor Green
    } catch {
        Write-Host "  [WARN] Could not download stb_image.h." -ForegroundColor Red
    }
} else {
    Write-Host "[2/3] stb_image.h already present." -ForegroundColor Green
}

# ?? 3. GLFW (source, built with CMake + MinGW) ??
$GLFW_Version = "3.4"
$GLFW_Src = Join-Path $ThirdParty "glfw"
New-Item -ItemType Directory -Force -Path $GLFW_Src | Out-Null
$GLFW_Build = Join-Path $BuildDir "glfw_build"

if (-not (Test-Path (Join-Path $GLFW_Src "CMakeLists.txt"))) {
    Write-Host "[3/3] Downloading GLFW $GLFW_Version..." -ForegroundColor Yellow
    $GLFW_Zip = Join-Path $BuildDir "glfw.zip"
    try {
        Invoke-WebRequest -Uri "https://github.com/glfw/glfw/releases/download/$GLFW_Version/glfw-$GLFW_Version.zip" -OutFile $GLFW_Zip -ErrorAction Stop
        Expand-Archive -Path $GLFW_Zip -DestinationPath $BuildDir -Force
        
        # Remove existing and copy source
        if (Test-Path $GLFW_Src) { Remove-Item -Path "$GLFW_Src\*" -Recurse -Force -ErrorAction SilentlyContinue }
        Copy-Item -Path (Join-Path $BuildDir "glfw-$GLFW_Version\*") -Destination $GLFW_Src -Recurse -Force
        Write-Host "  GLFW source downloaded." -ForegroundColor Green
        
        # Build GLFW
        Write-Host "  Building GLFW with MinGW..." -ForegroundColor Yellow
        New-Item -ItemType Directory -Force -Path $GLFW_Build | Out-Null
        Push-Location $GLFW_Build
        cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF $GLFW_Src
        mingw32-make -j$(nproc)
        Pop-Location
        Write-Host "  GLFW built." -ForegroundColor Green
    } catch {
        Write-Host "  [WARN] Could not download/build GLFW: $_" -ForegroundColor Red
    }
} else {
    Write-Host "[3/3] GLFW already present." -ForegroundColor Green
    # Build if not already built
    if (-not (Test-Path (Join-Path $GLFW_Build "src\libglfw3.a"))) {
        Write-Host "  Building GLFW..." -ForegroundColor Yellow
        New-Item -ItemType Directory -Force -Path $GLFW_Build | Out-Null
        Push-Location $GLFW_Build
        cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF $GLFW_Src
        mingw32-make -j$(nproc)
        Pop-Location
        Write-Host "  GLFW built." -ForegroundColor Green
    }
}

Write-Host "=== Setup complete ===" -ForegroundColor Cyan
Write-Host "Third-party location: $ThirdParty"
