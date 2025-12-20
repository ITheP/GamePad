@echo off

REM Runs powershell script accounting for execution policy

powershell.exe -ExecutionPolicy Bypass -File "crash.ps1"

pause