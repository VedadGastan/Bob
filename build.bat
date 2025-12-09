@echo off
setlocal enabledelayedexpansion

set COMPILER=g++
set FLAGS=-std=c++17 -Wall -Wextra -O2
set OUTPUT=build\bob.exe
set SOURCE=main.cpp

if "%1"=="" goto build
if "%1"=="build" goto build
if "%1"=="run" goto run
if "%1"=="repl" goto repl
if "%1"=="test" goto test
if "%1"=="clean" goto clean
goto help

:build
echo Building Bob interpreter...
if not exist build mkdir build
%COMPILER% %FLAGS% %SOURCE% -o %OUTPUT%
if %errorlevel% equ 0 (
    echo Build successful: %OUTPUT%
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
echo Running %2...
%OUTPUT% %2
goto end

:repl
if not exist %OUTPUT% call :build
%OUTPUT%
goto end

:test
if not exist %OUTPUT% call :build
echo Running all tests...
cd examples
call run_all_tests.bat
cd ..
goto end

:clean
if exist build rmdir /s /q build
echo Cleaned
goto end

:help
echo Usage: build.bat [command]
echo Commands:
echo   build       - Compile the interpreter
echo   run ^<file^>  - Run a Bob program
echo   repl        - Start REPL
echo   test        - Run all tests in examples folder
echo   clean       - Remove build folder
goto end

:end
endlocal