@echo off
setlocal enabledelayedexpansion

set COMPILER=g++
set FLAGS=-std=c++17 -Wall -Wextra -O2
set OUTPUT=build\bob.exe
set SOURCE=main.cpp

if "%1"=="" goto help
if "%1"=="build" goto build
if "%1"=="run" goto run
goto help

:build
if not exist build mkdir build
%COMPILER% %FLAGS% %SOURCE% -o %OUTPUT% 2>nul
if %errorlevel% equ 0 (
    echo Build successful
) else (
    echo Build failed
    exit /b 1
)
goto end

:run
if not exist %OUTPUT% call :build
if "%2"=="" (
    echo Usage: build.bat run ^<filename^>
    exit /b 1
)
%OUTPUT% %2
goto end

:help
echo Usage: build.bat [command]
echo.
echo Commands:
echo   build       - Compile the interpreter
echo   run ^<file^>  - Run a Bob program
echo.
echo Examples:
echo   build.bat build
echo   build.bat run program.bob
goto end

:end
endlocal