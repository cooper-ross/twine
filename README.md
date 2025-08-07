# Twine: Dynamic Language Compiler with LLVM

A high-performance compiler for a dynamic language, built in modern C++17 with direct LLVM IR generation and native machine code output. This is a true compiler, not a transpiler. Implements a full multi-stage pipeline including custom lexical analysis, recursive descent parsing, AST construction, type inference, and LLVM-based optimization. Features a dynamic typing system, memory-safe symbol management, and seamless C library integration!

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Prerequisites](#prerequisites)
- [Building the Compiler](#building-the-compiler)
- [Usage](#usage)
- [Language Syntax](#language-syntax)
- [API Reference](#api-reference)
- [Contributing](#contributing)

## Features

### Language Features

- **Variable Declarations**: `let`, `var`, and `const` keywords
- **Data Types**: 
  - Numbers (integers and floating-point)
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
  - **Math Functions**:
    - `abs(x)`: Absolute value
    - `round(x, [decimals])`: Round to nearest integer or decimal place
    - `min(a, b, ...)`: Find minimum value
    - `max(a, b, ...)`: Find maximum value
    - `pow(x, y)`: Raise x to power of y
    - `sqrt(x)`: Calculate square root
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
.\build.bat    # Output: twc.exe
```

#### Linux/macOS
```bash
chmod +x build.sh
./build.sh     # Output: ./twc
```

### Option 2: Using CMake

```bash
mkdir build
cd build
cmake ..
make     # Output: build/bin/twc
```

### Option 3: Direct Compilation

```bash
LLVM_FLAGS=$(llvm-config --cxxflags --ldflags --system-libs --libs core support irreader codegen mc mcparser option target)
g++ -std=c++17 -o twc main.cpp lexer.cpp parser.cpp ast.cpp codegen.cpp $LLVM_FLAGS
```

## Usage

### Basic Usage

```bash
# Compile a .tw file to executable
twc input.tw

# This creates:
# - input.ll (LLVM IR - deleted unless --verbose)
# - input.s (Assembly - deleted unless --verbose)
# - input.o (Object file - deleted unless --verbose)
# - input.exe (Windows) or input (Unix)
```

### Command-Line Options

```bash
twc <input.tw> [options]

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
twc fibonacci.tw -o fib

# Generate and inspect LLVM IR
twc program.tw --emit-ir
cat program.ll

# See all compilation steps
twc program.tw --verbose

# Generate optimized assembly
twc program.tw --emit-asm
```

## Language Syntax

### Variables

```javascript
let x = 10;           // Mutable variable
var y = "Hello";      // Mutable variable (same as let)
const PI = 4;         // Constant

// Variable reassignment
x = 20;
y = "World";

// Dynamic typing
let z = "Hello";      // String
z = 42;               // Now a number
z = true;             // Now a boolean

let sum = x + 5;
print(sum);
```

### Data Types

```javascript
// Numbers (stored as double-precision float)
let integer = 42;
let float = 3.14;
let negative = -17;

// Strings
let str1 = "Hello, World!";
let str2 = 'Single quotes work too';
let escaped = "Line 1\nLine 2\tTabbed";

// Booleans
let isTrue = true;
let isFalse = false;

// Null
let nothing = null;
```

### Operators

```javascript
// Arithmetic
let sum = 10 + 5;        // 15
let diff = 10 - 5;       // 5
let product = 10 * 5;    // 50
let quotient = 10 / 3;   // 3.333...
let remainder = 10 % 3;  // 1

// Comparison
let equal = 5 == 5;      // true
let notEqual = 5 != 3;   // true
let less = 3 < 5;        // true
let greater = 5 > 3;     // true
let lessEq = 3 <= 3;     // true
let greaterEq = 5 >= 5;  // true

// Logical
let and = true && false; // false
let or = true || false;  // true
let not = !true;         // false

// Complex expressions
let result = (10 + 5) * 2 - 3; // 27
```

### Control Flow

#### If/Else Statements

```javascript
let age = 18;

if (age >= 18) {
    printl("You are an adult");
} else {
    printl("You are a minor");
}

// Nested if/else
let score = 85;
if (score >= 90) {
    printl("Grade: A");
} else {
    if (score >= 80) {
        printl("Grade: B");
    } else {
        printl("Grade: C");
    }
}
```

#### While Loops

```javascript
let i = 0;
while (i < 5) {
    printl(i);
    i = i + 1;
}

// Infinite loop (be careful!)
while (true) {
    printl("Press Ctrl+C to stop");
}
```

#### For Loops

```javascript
// C-style for loop
for (let i = 0; i < 10; i = i + 1) {
    printl(i);
}

// Nested loops
for (let i = 0; i < 3; i = i + 1) {
    for (let j = 0; j < 3; j = j + 1) {
        printl(i * 3 + j);
    }
}
```

### Comments

```javascript
// This is a single-line comment

/*
   This is a
   multi-line comment
   spanning several lines
*/

let x = 5; // Comments can appear at end of lines

/* Comments can be /* nested */ in this compiler */
```

### Built-in Functions

The Twine language provides five built-in functions:

```javascript
// Print function - prints values with newline
print("Hello, World!");
print(42);
print(3.14159);

let name = "Alice";
print(name);
print(10 + 20);

// Input function - reads a full line of user input
print("What is your name?");
let userName = input(); // Can handle spaces, e.g., "John Doe"
print("Hello " + userName + "! Nice to meet you!");

// Example interactive program
print("Enter a number:");
let number = input();
print("You entered:");
print(number);

// Conversion functions
let number = 42;
let string = str(number); // "42"
print(string);

let text = "123.45";
let value = num(text); // 123.45
print(value);

let decimal = "99.7";
let integer = int(decimal); // 99 (truncated)
print(integer);

// Math functions
print("Absolute value: " + str(abs(-42))); // 42
print("Square root: " + str(sqrt(16))); // 4
print("2 raised to power 3: " + str(pow(2, 3))); // 8
print("Round 3.7: " + str(round(3.7))); // 4
print("Round PI to 2 decimals: " + str(round(3.14159, 2))); // 3.14
print("Min of 5 and 10: " + str(min(5, 10))); // 5
print("Max of values: " + str(max(3, 7, 2))); // 7

// Practical example
print("Enter your age:");
let ageStr = input();
let age = int(ageStr);
let nextYear = age + 1;
print("Next year you will be " + str(nextYear) + " years old!");
```

## API Reference

### Token Types

```cpp
enum class TokenType {
    // Literals
    NUMBER, STRING, IDENTIFIER,
    
    // Keywords
    LET, VAR, CONST, FUNCTION, IF, ELSE, 
    WHILE, FOR, RETURN, TRUE, FALSE, NULL_TOKEN,
    
    // Operators
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    ASSIGN, EQUAL, NOT_EQUAL, LESS_THAN, 
    GREATER_THAN, LESS_EQUAL, GREATER_EQUAL,
    LOGICAL_AND, LOGICAL_OR, LOGICAL_NOT,
    
    // Punctuation
    SEMICOLON, COMMA, DOT, 
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET,
    
    // Special
    END_OF_FILE, UNKNOWN
};
```

### AST Node Types

- **Expressions**: NumberLiteral, StringLiteral, BooleanLiteral, NullLiteral, Identifier, BinaryExpression, UnaryExpression, AssignmentExpression, CallExpression
- **Statements**: ExpressionStatement, VariableDeclaration, BlockStatement, IfStatement, WhileStatement, ForStatement, ReturnStatement, FunctionDeclaration
- **Program**: Root node containing all top-level statements

## Contributing

### Adding New Features

1. **New Operators**:
   - Add token type in `lexer.h`
   - Add lexing logic in `lexer.cpp`
   - Add parsing logic in `parser.cpp`
   - Add codegen logic in `codegen.cpp`

2. **New Statement Types**:
   - Define AST node in `ast.h`
   - Implement accept method in `ast.cpp`
   - Add parsing in `parser.cpp`
   - Add visitor method in `codegen.cpp`

3. **Built-in Functions**:
   - Add declaration in `declareBuiltinFunctions()`
   - Add special handling in `visit(CallExpression*)`

## License

This project is provided as-is for all purposes, and was really just for me - it's not amazing code, but if you want to use it, it's yours. Feel free to use, modify, and distribute as needed.

## Acknowledgments

* **The LLVM Project** – For providing excellent compiler infrastructure and tools.
* ***The Dragon Book*** – For foundational compiler theory and education.
* [Wisp by Adam McDaniel](https://github.com/adam-mcdaniel/wisp) – A minimal Lisp-like language built with LLVM.
* [LLVM IR with the C++ API – Tutorial by Mukul Rathi](https://mukulrathi.com/create-your-own-programming-language/llvm-ir-cpp-api-tutorial)
* [“Hello, LLVM” – Tutorial by James Hamilton](https://jameshamilton.eu/programming/llvm-hello-world)
* [Awesome LLVM](https://github.com/learn-llvm/awesome-llvm) – A curated list of useful LLVM resources.
* [Jesse Squires' Blog](https://www.jessesquires.com/blog/2020/12/28/resources-for-learning-about-compilers-and-llvm) – A great roundup of compiler and LLVM learning materials.