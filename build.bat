@echo off
setlocal

set "COMPILER=g++"
set "FLAGS=-std=c++17 -Wall -Wextra -O2"
set "OUTPUT=build\bob.exe"
set "SOURCE=main.cpp"

if /I "%1"=="build" goto :build
if /I "%1"=="run" goto :run
echo Usage: build.bat [build|run]
goto :eof

:build
if not exist build mkdir build
echo Compiling...

%COMPILER% %FLAGS% %SOURCE% -o %OUTPUT% > build.log 2>&1

if %errorlevel% equ 0 (
    echo Build successful.
    if exist build.log del build.log
) else (
    echo Build failed! Here are the errors:
    echo ---------------------------------
    type build.log
    echo ---------------------------------
    if exist build.log del build.log
    exit /b 1
)
exit /b 0

:run
if not exist "%OUTPUT%" (
    call :build
    if errorlevel 1 exit /b 1
)

if "%~2" == "" (
    echo Error: Missing filename. Usage: build.bat run ^<file^>
    exit /b 1
)

"%OUTPUT%" "%~2"
exit /b 0