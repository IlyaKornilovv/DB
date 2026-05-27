@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."
echo [1/4] Configuring DB project
if exist build rmdir /s /q build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b !errorlevel!

echo [2/4] Building C++ targets
cmake --build build --config Release
if errorlevel 1 exit /b !errorlevel!

echo [3/4] Publishing optional WPF GUI
if exist src\gui\DbWorkbench.Client.csproj (
  where dotnet >nul 2>nul
  if not errorlevel 1 (
    dotnet publish src\gui\DbWorkbench.Client.csproj -c Release --self-contained false -r win-x64 -o publish\win-x64
    if exist build\Release\customdb_server.exe copy build\Release\customdb_server.exe publish\win-x64\ >nul
  ) else (
    echo dotnet not found; skipping GUI publish
  )
)

echo [4/4] Docker image (optional)
where docker >nul 2>nul
if not errorlevel 1 docker build -t databaseoficial-customdb-server .

echo Build completed: build\Release\customdb_server.exe
endlocal
