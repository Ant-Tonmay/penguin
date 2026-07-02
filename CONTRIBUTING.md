# Contributing to aham

This project welcomes contributions to the language, runtime, VM, and tests.

## Development Setup

1. Clone and enter the repository.
2. Configure and build:

```bash
cmake -S . -B build
cmake --build build -j
```

3. Run test suite:

```bash
ctest --test-dir build --output-on-failure
```

## Contribution Workflow

1. Create a feature branch from `main`.
2. Make focused changes (one logical change per PR where possible).
3. Add or update tests for behavior changes.
4. Run full tests locally before opening a PR.
5. Open a PR with:
- What changed
- Why it changed
- How you tested it

## Where to Make Changes

- Lexer tokens/rules: `include/lexer/lexer.h`, `src/lexer/lexer.cpp`
- AST and parser: `include/parser/ast.h`, `include/parser/parser.h`, `src/parser/parser.cpp`
- Interpreter runtime: `include/interpreter/*`, `src/interpreter/*`
- VM backend/compiler: `include/vm/*`, `src/vm/*`
- Main CLI entrypoint: `src/main.cpp`

Detailed architecture map: `docs/ARCHITECTURE.md`

## Testing Expectations

When changing language behavior, update the relevant tests in `src/test_*.cpp`.

Common mappings:
- Lexing changes -> `src/test_lexer.cpp`
- Expression/statement/class parsing -> parser tests (`src/test_parser*.cpp`)
- Interpreter semantics -> `src/test_interpreter.cpp` (+ sample `.pg` inputs)
- VM semantics/opcodes -> `src/test_vm.cpp`
- Scope/symbol behavior -> `src/test_symbol_table.cpp`

Run targeted checks while iterating:

```bash
ctest --test-dir build -R LexerTest --output-on-failure
ctest --test-dir build -R Parser --output-on-failure
ctest --test-dir build -R VMTest --output-on-failure
```

## Coding Guidelines

- Use C++17-compatible code.
- Keep changes minimal and locally coherent.
- Prefer readable, explicit logic over clever shortcuts.
- Match existing style in surrounding files.
- Use clear runtime error messages for user-facing failures.

## PR Checklist

- [ ] Builds cleanly with CMake
- [ ] All tests pass (`ctest --test-dir build`)
- [ ] Added/updated tests for new behavior
- [ ] No unrelated refactors in same PR
- [ ] Documentation/examples updated when syntax or behavior changes

## Notes on Generated Files

Avoid committing generated build artifacts. Prefer out-of-source builds in `build/`.
