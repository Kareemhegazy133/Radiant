@echo off

rem %~dp0 = this script's directory, so the script works from any working directory
pushd "%~dp0.."

rem Check if bin folder exists, then delete
if exist bin rd /s /q bin

rem Check if bin-int folder exists, then delete
if exist bin-int rd /s /q bin-int

rem Run Premake
Tools\Binaries\Premake\Windows\premake5.exe --file=Build.lua vs2022
popd
pause
