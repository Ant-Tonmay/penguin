# aham Architecture

This document maps the current implementation so contributors can quickly find where to add or fix language features.

## High-Level Pipeline

1. Source file is read in `src/main.cpp`.
2. `Lexer` tokenizes source into `std::vector<Token>`.
3. `Parser` builds an AST (`Program`, `Function`, statements, expressions, classes).
4. Execution mode:
- Interpreter mode: `Interpreter::executeProgram(...)`
- VM mode (`--vm`): AST is compiled to bytecode (`vm::Compiler`), then executed by `vm::VM`

## Major Components

### Lexer

- Header: `include/lexer/lexer.h`
- Implementation: `src/lexer/lexer.cpp`

Responsibilities:
- Token definitions (`TokenType`)
- Keyword recognition
- Number/string/identifier scanning
- Operator tokenization (arithmetic, logical, bitwise, assignment variants)

### Parser and AST

- Parser API: `include/parser/parser.h`
- AST nodes: `include/parser/ast.h`
- Parser implementation: `src/parser/parser.cpp`

Responsibilities:
- Parse whole program `{ ... }` into `Program`
- Parse functions, statements, expressions
- Parse arrays, calls, member/index expressions
- Parse classes/sections/members/inheritance

### Interpreter

- Public API: `include/interpreter/interpreter.h`
- Implementations: `src/interpreter/*`

Responsibilities:
- Register user functions and classes
- Execute statements and evaluate expressions over environments
- Handle built-ins and runtime objects (arrays/classes/instances)

### VM Compiler + Runtime

- Compiler: `include/vm/compiler.h`, `src/vm/compiler_*.cpp`
- Runtime VM: `include/vm/vm.h`, `src/vm/vm*.cpp`
- Bytecode chunk/opcodes/value utilities in `include/vm/*`, `src/vm/*`

Responsibilities:
- Lower AST into bytecode (`Chunk` in `FunctionObject`)
- Execute opcodes with stack + call frames
- Support control flow, calls, arrays, classes, casts, and input ops

### Symbol Table

- API: `include/symbol_table/*`
- Implementation/tests: `src/symbol_table/symbol_table.cpp`, `src/test_symbol_table.cpp`

Responsibilities:
- Scoped symbol definition/get/assign behavior for supporting runtime logic

## Tests

CMake registers executable-based tests in `CMakeLists.txt`:
- Lexer: `src/test_lexer.cpp`
- Parser core/arrays/functions/loops/classes: `src/test_parser*.cpp`
- Interpreter: `src/test_interpreter.cpp`
- Symbol table: `src/test_symbol_table.cpp`
- VM: `src/test_vm.cpp`

Run all tests:

```bash
ctest --test-dir build --output-on-failure
```

## Adding a New Language Feature

Typical change flow:

1. Extend tokenization in lexer (if syntax requires new tokens).
2. Update AST definitions if new node kinds are needed.
3. Update parser to produce the new AST shape.
4. Implement runtime semantics:
- Interpreter path in `src/interpreter/*`
- VM path in compiler/VM files if feature should also work with `--vm`
5. Add or update tests that cover parsing + execution behavior.
6. Add/adjust example `.pg` programs in `examples/` if useful.

Keeping interpreter and VM semantics aligned is important when both backends support a construct.
