# Bob Programming Language

A simple, Python-inspired language with **true parallel programming** built-in.

## Quick Start

### Build
```bash
build.bat
```

### Run a program
```bash
build\bob.exe myfile.bob
```

## Syntax

### Variables
```ruby
var x = 10
var name = "Alice"
var list = [1, 2, 3, 4, 5]
```

### Operators
```bob
x += 5
x++
y *= 2
result = 2 ** 3
```

### Functions
```bob
func add(a, b)
    return a + b
end

func factorial(n)
    if n <= 1
        return 1
    end
    return n * factorial(n - 1)
end
```

### Control Flow
```bob
if x > 10
    print "Large"
else
    print "Small"
end

while x < 100
    x += 1
end

for (var i = 0; i < 10; i++)
    print i
end
```

### Arrays
```bob
var arr = [1, 2, 3, 4, 5]
print arr[0]
print arr[-1]
arr[2] = 99
print len(arr)
```

## The Gimmick: Parallel Programming

Bob's killer feature is **effortless parallel programming** that uses all your CPU cores.

### Basic Parallel Loop
```bob
var sum = 0
parallel i in 0..1000
    atomic_add("sum", i)
end
print sum
```

### Parallel Array Processing
```bob
var numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
var total = 0
parallel i in 0..len(numbers)
    atomic_add("total", numbers[i])
end
print "Sum: " + total
```

### Why It's Special
- **Automatic threading**: Bob handles all the complexity
- **CPU utilization**: Uses all available cores (reports with `num_threads()`)
- **Thread safety**: Use `atomic_add(var_name, value)` for safe concurrent operations
- **Simple syntax**: Just write `parallel` instead of `for`

### Parallel Built-ins
```bob
num_threads()           # Returns CPU core count
thread_id()            # Current thread ID
atomic_add("var", val) # Thread-safe addition
sleep(ms)              # Sleep milliseconds
```

## Built-in Functions
```bob
print "Hello"          # Print to console
len(array)             # Length of array/string
sqrt(x)                # Square root
pow(x, y)              # Power
abs(x)                 # Absolute value
floor(x), ceil(x)      # Rounding
sin(x), cos(x), tan(x) # Trigonometry
range(start, end)      # Create array range
```

## Examples

### Fibonacci
```bob
func fib(n)
    if n <= 1
        return n
    end
    return fib(n - 1) + fib(n - 2)
end

print fib(10)
```

### Parallel Sum of Squares
```bob
var result = 0
parallel i in 1..1001
    atomic_add("result", i * i)
end
print "Sum of squares 1-1000: " + result
```

### String Operations
```bob
var text = "Hello" + " World"
var repeated = "Bob " * 3
if "el" in "Hello"
    print "Found it!"
end
```