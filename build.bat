@echo off
echo Building Twine Compiler...

REM Check if we have the necessary tools
where llvm-config >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: llvm-config not found in PATH
    echo Please install LLVM and ensure llvm-config is accessible
    pause
    exit /b 1
)

where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: g++ not found in PATH
    echo Please install MinGW-w64 or similar C++ compiler
    pause
    exit /b 1
)

echo Compiling Twine Compiler with g++...

REM Get LLVM flags
for /f %%i in ('llvm-config --cxxflags --ldflags --system-libs --libs core support irreader codegen mc mcparser option target') do set LLVM_FLAGS=%%i

REM Compile with proper include path
g++ -std=c++17 -Iinclude -o twine.exe src/main.cpp src/lexer.cpp src/parser.cpp src/ast.cpp src/codegen.cpp %LLVM_FLAGS%

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build successful!
echo Compiler is at: twine.exe
echo.
echo Example usage:
echo   twine.exe examples\fibonacci.tw
echo   twine.exe examples\math.tw --verbose
echo.
pause