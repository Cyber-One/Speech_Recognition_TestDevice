# Build Script for I2C_TestDevice
# Quick build script to compile the I2C Test Device project

$BuildDir = "$PSScriptRoot\build"

Write-Host "Building I2C_TestDevice..." -ForegroundColor Cyan

# Create build directory if it doesn't exist
if (-not (Test-Path $BuildDir)) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
    
    # Run CMake configuration
    Write-Host "Running CMake configuration..." -ForegroundColor Yellow
    Push-Location $BuildDir
    cmake ..
    Pop-Location
}

# Build using Ninja
Write-Host "Compiling with Ninja..." -ForegroundColor Yellow
& "$env:USERPROFILE\.pico-sdk\ninja\v1.12.1\ninja.exe" -C $BuildDir

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nBuild successful!" -ForegroundColor Green
    Write-Host "Output file: $BuildDir\I2C_TestDevice.uf2" -ForegroundColor Green
    
    # Show file size
    $uf2File = Get-Item "$BuildDir\I2C_TestDevice.uf2"
    Write-Host "File size: $($uf2File.Length) bytes" -ForegroundColor Cyan
} else {
    Write-Host "`nBuild failed with exit code: $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}
