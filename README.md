# Twine: Dynamic Language Compiler with LLVM

A high-performance compiler for a dynamic language, built in C++17 with direct LLVM IR generation and native machine code output. This is a true compiler, not a transpiler, and it implements a full multi-stage pipeline including custom lexical analysis, recursive descent parsing, AST construction, type inference, and LLVM-based optimization. It also features a dynamic typing system, memory-safe symbol management, and seamless C library integration! Built entirely to help me learn more about compiler design and lower level programming.

## Table of Contents

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
    print("You are an adult");
} else {
    print("You are a minor");
}

// Nested if/else
let score = 85;
if (score >= 90) {
    print("Grade: A");
} else {
    if (score >= 80) {
        print("Grade: B");
    } else {
        print("Grade: C");
    }
}
```

#### While Loops

```javascript
let i = 0;
while (i < 5) {
    print(i);
    i = i + 1;
}

// Infinite loop (be careful!)
while (true) {
    print("Press Ctrl+C to stop");
}
```

#### For Loops

```javascript
// C-style for loop
for (let i = 0; i < 10; i = i + 1) {
    print(i);
}

// Nested loops
for (let i = 0; i < 3; i = i + 1) {
    for (let j = 0; j < 3; j = j + 1) {
        print(i * 3 + j);
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

// String functions
let message = "Hello, World!";
let messageLength = len(message); // 13
print("Message length: " + str(messageLength));

let uppercaseMessage = upper(message); // "HELLO, WORLD!"
print("Uppercase: " + uppercaseMessage);

let lowercaseMessage = lower(message); // "hello, world!"
print("Lowercase: " + lowercaseMessage);

// String search function
let text = "The quick brown fox jumps over the lazy dog";
let hasQuick = includes(text, "quick"); // 1.0 (true)
let hasCat = includes(text, "cat"); // 0.0 (false)
print("Contains 'quick': " + str(hasQuick));
print("Contains 'cat': " + str(hasCat));

// String replacement function
let original = "Hello, World!";
let replaced = replace(original, "World", "Universe"); // "Hello, Universe!"
let noChange = replace(original, "xyz", "abc"); // "Hello, World!" (no change)
print("Original: " + original);
print("Replaced: " + replaced);
print("No change: " + noChange);

// Arrays
let numbers = [1, 2, 3, 4, 5];
print("Array: " + str(numbers[0]) + ", " + str(numbers[1])); // "Array: 1, 2"
print("Array length: " + str(len(numbers))); // "Array length: 5"

// Array indexing
let value = numbers[2]; // 3
numbers[2] = 10; // Modify array element

// Dynamic arrays with append
let items = [10, 20];
let moreItems = append(items, 30); // [10, 20, 30]
let evenMore = append(moreItems, 40); // [10, 20, 30, 40]

// Math functions
print("Absolute value: " + str(abs(-42))); // 42
print("Square root: " + str(sqrt(16))); // 4
print("2 raised to power 3: " + str(pow(2, 3))); // 8
print("Round 3.7: " + str(round(3.7))); // 4
print("Round PI to 2 decimals: " + str(round(3.14159, 2))); // 3.14
print("Floor 3.7: " + str(floor(3.7))); // 3
print("Ceil 3.2: " + str(ceil(3.2))); // 4
print("Min of 5 and 10: " + str(min(5, 10))); // 5
print("Max of values: " + str(max(3, 7, 2))); // 7
print("Random number: " + str(random())); // e.g., 0.573821
print("Random dice roll (1-6): " + str(int(str(random() * 6 + 1)))); // e.g., 4

// Practical example
print("Enter your age:");
let ageStr = input();
let age = int(ageStr);
let nextYear = age + 1;
print("Next year you will be " + str(nextYear) + " years old!");
```

### User-Defined Functions

Twine supports user-defined functions with full recursion, parameter passing, and return values.

#### Function Declaration

```javascript
// Simple function with no parameters
function getAnswer() {
    return 42;
}

// Function with parameters
function add(a, b) {
    return a + b;
}

// Function with multiple parameters
function pythagorean(a, b) {
    return sqrt(a * a + b * b);
}
```

#### Recursive Functions

```javascript
// Basic recursion
function factorial(n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

// Mutual recursion
function isEven(n) {
    if (n == 0) {
        return 1;
    } else {
        return isOdd(n - 1);
    }
}

function isOdd(n) {
    if (n == 0) {
        return 0;
    } else {
        return isEven(n - 1);
    }
}
```

#### Functions with Control Flow

```javascript
// Function with loops and local variables
function sumRange(start, end) {
    let total = 0;
    for (let i = start; i <= end; i = i + 1) {
        total = total + i;
    }
    return total;
}

// Function with conditional logic
function max(a, b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}
```

#### Function Calls

```javascript
// Calling functions
let result = add(15, 27); // 42
let hypotenuse = pythagorean(3, 4); // 5
let fact5 = factorial(5); // 120

// Nested function calls
let complex = add(factorial(3), pythagorean(3, 4)); // 6 + 5 = 11

// Functions in expressions
let area = pythagorean(3, 4) * 2; // 10
```

#### Function Features

- **Dynamic Typing**: Functions can return different types based on conditions
- **Variable Scope**: Local variables are scoped to the function
- **Recursion**: Full support for recursive and mutually recursive functions
- **Parameter Passing**: Pass by value for all parameters
- **Return Values**: Functions can return any data type or no value (null)
- **Built-in Integration**: Functions can call built-in functions seamlessly

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

1. **New Statement Types**:
   - Define AST node in `ast.h`
   - Implement accept method in `ast.cpp`
   - Add parsing in `parser.cpp`
   - Add visitor method in `codegen.cpp`

2. **Built-in Functions**:
   - Add declaration in `declareBuiltinFunctions()`
   - Add special handling in `visit(CallExpression*)`

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