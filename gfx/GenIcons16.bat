@echo off

REM Note this was originally a dos script, but powershell did things so much faster
REM Still need this batch file to kick off the powershell script
REM Script takes 2 parameters...
REM - Name of icon file
REM - Name of resulting font
REM Will generate into equivalent named .h file

powershell.exe -ExecutionPolicy Bypass -File "GenIcons16.ps1" "CustomIcons.16x16.pbm" "CustomIcons16"
powershell.exe -ExecutionPolicy Bypass -File "GenIcons16.ps1" "CustomLogos.16x16.pbm" "CustomLogos16"