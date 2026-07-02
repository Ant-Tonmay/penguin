cmake_minimum_required(VERSION 3.16)

project(
    aham
    VERSION 0.1.0
    DESCRIPTION "Aham Programming Language"
    LANGUAGES CXX
)

# -----------------------------
# C++ standard
# -----------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
# Core library
# -----------------------------
add_library(aham_core
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/interpreter/interpreter.cpp
    src/interpreter/environment.cpp
    src/interpreter/expr_evaluator.cpp
    src/interpreter/stmt_executor.cpp
    src/interpreter/deep_copy.cpp
    src/symbol_table/symbol_table.cpp
    src/compiler/chunk.cpp
    src/compiler/compiler_core.cpp
    src/compiler/compiler_expr.cpp
    src/compiler/compiler_stmt.cpp
    src/vm/vm.cpp
    src/vm/vm_array.cpp
    src/vm/vm_call.cpp
    src/vm/vm_class.cpp
    src/vm/vm_ops.cpp
    src/vm/vm_trait.cpp
    src/vm/vm_builtins.cpp
    src/vm/vm_helper.cpp
    src/vm/utils/access_utils.cpp
    src/vm/utils/value_utils.cpp
    src/compiler/utils/serializer.cpp
    src/compiler/utils/deserializer.cpp
    src/loader/module_loader.cpp
    src/utils/utils.cpp
    src/dependency_graph/dependency_graph.cpp
    src/dependency_graph/dependency_scanner.cpp
    src/manifest/manifest.cpp
    src/manifest/manifest_loader.cpp
    src/manifest/manifest_parser.cpp

)

target_include_directories(aham_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# -----------------------------
# Compiler warnings
# -----------------------------
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(aham_core PRIVATE -Wall -Wextra -Wpedantic)
elseif (MSVC)
    target_compile_options(aham_core PRIVATE /W4)
endif()

# -----------------------------
# Main executable
# -----------------------------
add_executable(aham src/main.cpp)
target_link_libraries(aham PRIVATE aham_core)

# -----------------------------
# Tests
# -----------------------------
enable_testing()

add_executable(aham_lexer_test tests/test_lexer.cpp)
target_link_libraries(aham_lexer_test PRIVATE aham_core)
add_test(NAME LexerTest COMMAND aham_lexer_test)

add_executable(aham_parser_test tests/test_parser.cpp)
target_link_libraries(aham_parser_test PRIVATE aham_core)
add_test(NAME ParserTest COMMAND aham_parser_test)

add_executable(aham_symbol_table_test tests/test_symbol_table.cpp)
target_link_libraries(aham_symbol_table_test PRIVATE aham_core)
add_test(NAME SymbolTableTest COMMAND aham_symbol_table_test)

add_executable(aham_interpreter_test tests/test_interpreter.cpp)
target_link_libraries(aham_interpreter_test PRIVATE aham_core)
add_test(NAME InterpreterTest COMMAND aham_interpreter_test ${PROJECT_SOURCE_DIR}/examples/operators.pg)


add_executable(aham_parser_array_test tests/test_parser_arrays.cpp)
target_link_libraries(aham_parser_array_test PRIVATE aham_core)
add_test(NAME ParserArrayTest COMMAND aham_parser_array_test)

add_executable(aham_parser_function_test tests/test_parser_functions.cpp)
target_link_libraries(aham_parser_function_test PRIVATE aham_core)
add_test(NAME ParserFunctionTest COMMAND aham_parser_function_test)

add_executable(aham_parser_loops_test tests/test_parser_loops.cpp)
target_link_libraries(aham_parser_loops_test PRIVATE aham_core)
add_test(NAME ParserLoopsTest COMMAND aham_parser_loops_test)

add_executable(aham_parser_classes_test tests/test_parser_classes.cpp)
target_link_libraries(aham_parser_classes_test PRIVATE aham_core)
add_test(NAME ParserClassesTest COMMAND aham_parser_classes_test)

# -----------------------------
# Debug flags
# -----------------------------
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(aham_core PRIVATE DEBUG)
endif()
