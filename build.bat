@echo off
setlocal
cd /d "%~dp0"

set ARGS=%*
if /i "%~1"=="vc6" goto vc6
if /i "%~1"=="x86" set ARGS=%ARGS:*x86=%& goto x86
if /i "%~1"=="x64" set ARGS=%ARGS:*x64=%& goto x64
if /i "%PROCESSOR_ARCHITECTURE%"=="x86" if not defined PROCESSOR_ARCHITEW6432 goto x86

:x64
set PRESET=windows-x64
set OUTPUT=Wow64.exe
goto preset

:x86
set PRESET=windows-x86
set OUTPUT=Wow.exe

:preset
cmake --preset %PRESET% %ARGS%
if errorlevel 1 exit /b 1
cmake --build --preset %PRESET%-release --parallel
if errorlevel 1 exit /b 1
echo Built %CD%\Build\%PRESET%\WoW\bin\Release\%OUTPUT%
exit /b

:vc6
set ARGS=%ARGS:*vc6=%
call "C:\Program Files (x86)\Microsoft Visual Studio 6.0\VC98\Bin\VCVARS32.BAT"
cd Build
cmake -DCMAKE_BUILD_TYPE=Release_Assertions_Enabled -G "NMake Makefiles" %ARGS% ..
if errorlevel 1 exit /b 1
nmake
if errorlevel 1 exit /b 1
echo Built %CD%\WoW\bin\Wowae.exe
