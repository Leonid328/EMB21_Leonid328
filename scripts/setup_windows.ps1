# setup_windows.ps1 - Install required dependencies for Windows (requires winget)

Write-Host "Installing Windows dependencies..." -ForegroundColor Green

# Check if winget is available
if (!(Get-Command "winget" -ErrorAction SilentlyContinue)) {
    Write-Host "winget is not installed! Please install App Installer from Microsoft Store." -ForegroundColor Red
    exit 1
}

winget install --id Python.Python.3.11 --accept-package-agreements --accept-source-agreements
winget install --id LLVM.LLVM --accept-package-agreements --accept-source-agreements
winget install --id Git.Git --accept-package-agreements --accept-source-agreements
winget install --id Kitware.CMake --accept-package-agreements --accept-source-agreements
winget install --id ARM.GnuArmEmbeddedToolchain --accept-package-agreements --accept-source-agreements

# Install PlatformIO
Write-Host "Installing PlatformIO..." -ForegroundColor Green
pip install -U platformio

Write-Host "Setup complete! Please restart your terminal/VS Code." -ForegroundColor Green
