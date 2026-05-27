$ErrorActionPreference = "Stop"
Write-Host "Cleaning old GUI build..."
dotnet clean
Write-Host "Building GUI..."
dotnet build
Write-Host "Starting GUI..."
dotnet run
