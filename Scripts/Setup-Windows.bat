@echo off

rem Check if bin folder exists, then delete
pushd ..
if exist bin rd /s /q bin

rem Check if bin-int folder exists, then delete
if exist bin-int rd /s /q bin-int

rem Run Premake
Vendor\Binaries\Premake\Windows\premake5.exe --file=Build.lua vs2022
popd
pause
