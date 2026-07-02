# Aham Programming Language

Penguin is a C++-implemented programming language with:
- A lexer and parser that build an AST
- A tree-walk interpreter
- A bytecode compiler + virtual machine backend (`--vm`)

The repository includes language examples, parser/interpreter/VM tests, and the core runtime implementation.

## Current Feature Snapshot

Based on the current source and tests, Penguin supports:
- Functions (`func`), `return`, call-by-value and `ref` parameters
- Variables, assignment, arithmetic, comparison, logical and bitwise operators
- Control flow: `if`/`else`, `for`, `while`, `break`, `continue`
- Arrays (dynamic and `fixed(size[, init])`)
- Classes, fields, methods, access blocks (`public`/`private`/`protected`), and inheritance
- Built-in utilities including `print`, `println`, `readline`, `type`, and casts like `int(...)`

## Prerequisites

- CMake `>= 3.16`
- A C++17 compiler (`g++` or `clang++`)
- Make/Ninja generator support for CMake

## Build

Use an out-of-source build:

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

Interpreter mode:

```bash
./build/penguin examples/hello.pg
```

VM mode:

```bash
./build/penguin --vm examples/hello.pg
```

CLI flags:

```bash
./build/penguin --help
./build/penguin --version
./build/penguin --info
```

## Test

Run all tests:

```bash
ctest --test-dir build --output-on-failure
```

Run a single test target:

```bash
ctest --test-dir build -R ParserTest --output-on-failure
```

Main registered tests:
- `LexerTest`
- `ParserTest`
- `SymbolTableTest`
- `InterpreterTest`
- `ParserArrayTest`
- `ParserFunctionTest`
- `ParserLoopsTest`
- `ParserClassesTest`
- `VMTest`

## Project Layout

```text
include/      Public headers for lexer, parser, interpreter, symbol table, vm
src/          Implementations + test executables
examples/     Example Penguin programs (*.pg)
tests/        Extra test programs and fixtures
grammar/      Grammar notes and language design artifacts
goals/        Planning notes and issues
```

## Examples

- [examples/hello.pg](examples/hello.pg)
- [examples/functions.pg](examples/functions.pg)
- [examples/array.pg](examples/array.pg)
- [examples/class_test.pg](examples/class_test.pg)
- [examples/keyboard.pg](examples/keyboard.pg)

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for setup, workflow, testing expectations, and PR guidelines.

For architecture and implementation map, see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).
