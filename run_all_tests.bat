@echo off
cd /d "%~dp0"
call scripts\build_full.bat
if errorlevel 1 exit /b %errorlevel%
build\tests\Release\unit_tests.exe
if errorlevel 1 exit /b %errorlevel%
build\tests\Release\integration_tests.exe
if errorlevel 1 exit /b %errorlevel%
python functional_test.py
