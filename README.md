# Twine: Dynamic Language Compiler with LLVM

A high-performance compiler for a dynamic language, built in C++17 with direct LLVM IR generation and native machine code output. It's a true compiler, not a transpiler, so it implements a full multi-stage pipeline including custom lexical analysis, recursive descent parsing, AST construction, type inference, and LLVM-based optimization.

## Table of Contents

- [Features](#features)
- [Building the Compiler](#building-the-compiler)
- [Usage](#usage)

## Features

### Language Features

- **Variable Declarations**: `let`, `var`, and `const` keywords
- **Data Types**: 
  - Numbers (integers and floating-point)
  - Arrays (dynamic, homogeneous)
  - Strings (with escape sequences)
  - Booleans (`true`/`false`)
  - Null values
- **Operators**:
  - Arithmetic: `+`, `-`, `*`, `/`, `%`
  - Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
  - Logical: `&&`, `||`, `!`
  - Assignment: `=`
- **Control Flow**:
  - `if`/`else` statements
  - `while` loops
  - `for` loops with C-style syntax
- **Built-in Functions**:
  - `print(value)`: Print value with newline
  - `input()`: Read input from user
  - `str(number)`: Convert number to string
  - `num(string)`: Convert string to number  
  - `int(string)`: Convert string to integer
- **String Functions**:
  - `len(string)`: Return the length of a string
  - `upper(string)`: Convert string to uppercase
  - `lower(string)`: Convert string to lowercase
  - `includes(haystack, needle)`: Check if haystack contains needle (returns 1.0 or 0.0)
  - `replace(haystack, old, new)`: Replace first occurrence of old with new in haystack
- **Array Functions**:
  - `len(array)`: Return the length of an array
  - `append(array, value)`: Append value to array and return new array
  - `includes(haystack, needle)`: Check if haystack array contains needle (returns 1.0 or 0.0)
- **Math Functions**:
  - `abs(x)`: Absolute value
  - `round(x, [decimals])`: Round to nearest integer or decimal place
  - `floor(x)`: Round down to nearest integer
  - `ceil(x)`: Round up to nearest integer
  - `sin(x)`: Sine of x (radians)
  - `cos(x)`: Cosine of x (radians)
  - `tan(x)`: Tangent of x (radians)
  - `min(a, b, ...)`: Find minimum value
  - `max(a, b, ...)`: Find maximum value
  - `pow(x, y)`: Raise x to power of y
  - `sqrt(x)`: Calculate square root
  - `random()`: Generate random number from 0 to 1
- **Custom Functions**:
  - User-defined functions with parameters and return values
  - Full recursion support (including mutual recursion)
  - Local variable scope within functions
  - Integration with built-in functions
- **Comments**:
  - Single-line: `// comment`
  - Multi-line: `/* comment */`

### Compiler Features

- **Complete Pipeline**: Lexer → Parser → AST → LLVM IR → Native Code
- **Error Handling**: Syntax error reporting with line/column information
- **Optimization**: Automatic optimization via LLVM's opt tool
- **Multiple Output Formats**: Can emit LLVM IR, assembly, object files, or executables
- **Verbose Mode**: See each compilation step

## Prerequisites

### Required Software

1. **C++ Compiler**: 
   - GCC 7.0+ or Clang 5.0+ with C++17 support
   - On Windows: MinGW-w64 recommended

2. **LLVM**: 
   - Version 10.0 or higher (tested with 20.1.8)
   - Must be in PATH or specify LLVM_DIR for CMake

3. **Build Tools**:
   - CMake 3.10+ (optional but recommended)
   - GNU Make or MinGW Make (for CMake builds)

4. **Linker**:
   - GNU ld or compatible (usually comes with GCC)

### Verifying Prerequisites

```bash
g++ --version         # C++ compiler
llvm-config --version # LLVM
llc --version         # LLVM
cmake --version       # Cmake (Optional)
ld --version          # Linker
```

## Building the Compiler

### Option 1: Using Build Scripts (Recommended)

#### Windows
```bash
.\build.bat    # Output: twine.exe
```

#### Linux/macOS
```bash
chmod +x build.sh
./build.sh     # Output: ./twine
```

### Option 2: Using CMake

```bash
mkdir build
cd build
cmake ..
make     # Output: build/bin/twine
```

### Option 3: Direct Compilation

```bash
LLVM_FLAGS=$(llvm-config --cxxflags --ldflags --system-libs --libs core support irreader codegen mc mcparser option target)
g++ -std=c++17 -o twine main.cpp lexer.cpp parser.cpp ast.cpp codegen.cpp $LLVM_FLAGS
```

## Usage

### Basic Usage

```bash
# Compile a .tw file to executable
twine input.tw

# This creates:
# - input.ll (LLVM IR - deleted unless --verbose)
# - input.s (Assembly - deleted unless --verbose)
# - input.o (Object file - deleted unless --verbose)
# - input.exe (Windows) or input (Unix)
```

### Command-Line Options

```bash
twine <input.tw> [options]

Options:
  -o <output>      Specify output executable name
  --emit-ir        Output LLVM IR only (.ll file)
  --emit-asm       Output assembly only (.s file)
  --emit-obj       Output object file only (.o file)
  --verbose        Show all compilation steps and keep intermediate files
  --help           Display help message
  --version        Show version information
```

### Examples

```bash
# Compile with custom output name
twine fibonacci.tw -o fib

# Generate and inspect LLVM IR
twine program.tw --emit-ir
cat program.ll

# See all compilation steps
twine program.tw --verbose

# Generate optimized assembly
twine program.tw --emit-asm
```

## License

This project is provided as-is for all purposes, and was really just for me - it's not amazing code, but if you want to use it, it's yours. Feel free to use, modify, and distribute as needed.

## Acknowledgments

* [The LLVM Project](https://llvm.org) - For providing excellent compiler infrastructure and tools.
* [The Dragon Book](https://faculty.sist.shanghaitech.edu.cn/faculty/songfu/cav/Dragon-book.pdf) - For foundational compiler theory, and lots on lexing and parsing.
* [Wisp by Adam McDaniel](https://github.com/adam-mcdaniel/wisp) - A minimal Lisp-like language built with LLVM.
* [LLVM IR with the C++ API](https://mukulrathi.com/create-your-own-programming-language/llvm-ir-cpp-api-tutorial) - Tutorial by Mukul Rathi
* [“Hello, LLVM”](https://jameshamilton.eu/programming/llvm-hello-world) - Tutorial by James Hamilton
* [Awesome LLVM](https://github.com/learn-llvm/awesome-llvm) - A curated list of useful LLVM resources.
* [Jesse Squires' Blog](https://www.jessesquires.com/blog/2020/12/28/resources-for-learning-about-compilers-and-llvm) - A great roundup of compiler and LLVM learning materials.
* [Building an Optimizing Compiler](https://turbo51.com/download/Building-an-Optimizing-Compile-Book-Preview.pdf) - Old, but still very good, helpful with high level design.