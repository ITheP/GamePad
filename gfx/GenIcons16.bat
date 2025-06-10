@echo off

REM Note this was originally a dos script, but powershell did things so much faster
REM Still need this batch file to kick off the powershell script

powershell.exe -ExecutionPolicy Bypass -File "GenIcons16.ps1"
