# Bob Programming Language

A dynamically-typed, interpreted programming language with built-in parallel execution support, inspired by Python and C++ syntax.

## Overview

Bob is an educational programming language developed for the Programming Languages and Compilers course at the Faculty of Electrical Engineering, University of Sarajevo. It features a tree-walking interpreter implemented in C++.

## Features

- **Dynamic typing** - types determined at runtime
- **First-class functions** - functions as values with closure support
- **Parallel execution** - built-in `parallel` loops with thread-safe operations
- **Simple syntax** - optional semicolons, familiar control structures
- **Interactive REPL** - for quick experimentation

## Installation

```bash
# Clone the repository
git clone https://github.com/VedadGastan/Bob.git
cd Bob
```

### Building and Running

**Build the compiler:**
```bash
build.bat build
```

**Run a Bob script:**
```bash
build.bat run examples/script.bob
```

**Run REPL (interactive mode):**
```bash
build\bob.exe
```

The build script uses the following compiler flags:
- `-std=c++17` - C++17 standard
- `-Wall -Wextra` - all warnings enabled
- `-O2` - optimization level 2

## Quick Start

### Hello World

```bob
print("Hello, World!")
```

### Variables

```bob
var x = 10
var name = "Bob"
var numbers = [1, 2, 3, 4, 5]
```

Semicolons are optional:

```bob
var x = 10;
var y = 20;
print(x + y)  // works without semicolon too
```

### Functions

```bob
func add(a, b) {
    return a + b
}

func greet(name) {
    print("Hello, " + name)
}

var result = add(5, 3)
greet("Alice")
```

### Closures

```bob
func makeCounter() {
    var count = 0
    
    func increment() {
        count++
        return count
    }
    
    return increment
}

var counter = makeCounter()
print(counter())  // 1
print(counter())  // 2
```

### Control Flow

```bob
// If-elif-else
if (x > 10) {
    print("Greater than 10")
} elif (x > 5) {
    print("Greater than 5")
} else {
    print("5 or less")
}

// While loop
var i = 0
while (i < 5) {
    print(i)
    i++
}

// For loop
for (var i = 0; i < 10; i++) {
    print(i)
}
```

### Arrays

```bob
var arr = [1, 2, 3, 4, 5]
print(arr[0])      // 1
print(arr[-1])     // 5 (negative indexing)

arr[2] = 99        // modify element
push(arr, 6)       // add to end
var last = pop(arr) // remove from end

print(len(arr))    // length
```

### Parallel Execution

```bob
var sum = 0
atomic_store("sum", 0)

parallel (var i = 1; i <= 100; i++) {
    atomic_add("sum", i)
}

print(atomic_load("sum"))  // 5050
```

## Data Types

| Type | Description | Example |
|------|-------------|---------|
| `nil` | Null/undefined value | `var x` |
| `bool` | Boolean | `true`, `false` |
| `number` | Double-precision float | `42`, `3.14` |
| `string` | Text | `"Hello"` |
| `array` | Dynamic array | `[1, 2, 3]` |
| `function` | First-class function | `func add(a,b) {...}` |

## Operators

### Arithmetic
`+` `-` `*` `/` `%` `**` (exponentiation)

### Comparison
`==` `!=` `<` `<=` `>` `>=`

### Logical
`and` `or` `not`

### Assignment
`=` `+=` `-=` `*=` `/=` `%=`

### Increment/Decrement
`++` `--` (postfix only)

### Membership
`in` (check if element in array/string)

## Built-in Functions

### I/O
- `print(...)` - print to stdout
- `input(prompt)` - read from stdin

### Math
- `sqrt(x)`, `pow(x, y)`, `abs(x)`
- `floor(x)`, `ceil(x)`, `round(x)`
- `sin(x)`, `cos(x)`, `tan(x)`, `log(x)`
- `random()` - random number [0, 1)

### Array
- `len(x)` - length of array/string
- `push(arr, val)` - add to end
- `pop(arr)` - remove from end

### Utility
- `time()` - current time in milliseconds
- `sleep(ms)` - pause execution

### Parallel
- `thread_id()` - current thread ID
- `num_threads()` - number of available threads
- `atomic_store(name, val)` - atomic store
- `atomic_load(name)` - atomic load
- `atomic_add(name, val)` - atomic addition
- `atomic_inc(name)` - atomic increment
- `atomic_dec(name)` - atomic decrement
- `atomic_xchg(name, val)` - atomic exchange
- `atomic_cas(name, expected, new)` - compare-and-swap

## Examples

### Fibonacci

```bob
func fib(n) {
    if (n <= 1) {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

for (var i = 0; i < 10; i++) {
    print(fib(i))
}
```

### Filter Even Numbers

```bob
func filterEven(arr) {
    var result = []
    for (var i = 0; i < len(arr); i++) {
        if (arr[i] % 2 == 0) {
            push(result, arr[i])
        }
    }
    return result
}

var numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
var evens = filterEven(numbers)
print(evens)  // [2, 4, 6, 8, 10]
```

### Parallel Sum of Squares

```bob
var n = 1000
atomic_store("sum", 0)

parallel (var i = 1; i <= n; i++) {
    atomic_add("sum", i * i)
}

print("Sum of squares: " + atomic_load("sum"))
```

## Architecture

Bob uses a **tree-walking interpreter** with four main components:

1. **Lexer** - tokenizes source code
2. **Parser** - builds Abstract Syntax Tree (AST)
3. **AST** - represents program structure
4. **Interpreter** - directly executes AST

### Memory Management
- Uses C++ `std::shared_ptr` for automatic memory management
- Reference counting handles deallocation

### Threading
- Parallel loops use `std::thread` library
- Number of threads determined by `std::thread::hardware_concurrency()`
- Thread-safe global variable access via `std::mutex`

## Language Characteristics

- **Dynamically typed** - no type declarations, runtime type checking
- **Lexically scoped** - closures capture surrounding environment
- **Interpreted** - no compilation, direct AST execution
- **Garbage collected** - via reference counting with shared pointers

## Limitations

- No `break`/`continue` statements (yet)
- No exception handling
- No module/import system
- No classes/objects
- No lambda expressions
- Parallel loops require simple structure (simple init/condition/increment)

## Error Handling

Bob reports errors with line and column information:

```
[line 5:10] RuntimeError: Undefined variable 'x'
[line 3:5] ParseError: Expect ')' after expression
```

## REPL Mode

Start the interactive interpreter:

```bash
$ build\bob.exe
Bob Language REPL v1.0
Type 'exit' to quit

>>> var x = 10
>>> print(x * 2)
20
>>> exit
```

## Authors

- Vedad Gaštan (19685)
- Naila Delalić (19687)

**Faculty of Electrical Engineering, University of Sarajevo**  
*Programming Languages and Compilers Course*

---

**Note**: Bob is an educational programming language designed for learning interpreter implementation. It is not intended for production use.