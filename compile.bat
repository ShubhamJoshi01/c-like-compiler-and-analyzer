@echo off
echo Compiling C++ Analyzer Core...
g++ -std=c++17 compiler/main.cpp -o analyzer.exe
if %ERRORLEVEL% equ 0 (
    echo Compilation successful: analyzer.exe created.
) else (
    echo Compilation failed.
)
