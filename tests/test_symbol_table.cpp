#include <cassert>
#include <iostream>
#include <string>
#include "symbol_table/symbol_table.h"
#include "symbol_table/value.h"

void test_basic_define_and_get() {
    SymbolTable st;
    st.define("x", 10);
    st.define("y", 20);

    assert(std::get<int>(st.get("x")) == 10);
    assert(std::get<int>(st.get("y")) == 20);
    std::cout << "test_basic_define_and_get passed" << std::endl;
}

void test_scope() {
    SymbolTable global;
    global.define("x", 10);

    SymbolTable local(&global);
    local.define("y", 20);

    assert(std::get<int>(local.get("x")) == 10);
    assert(std::get<int>(local.get("y")) == 20);
    std::cout << "test_scope passed" << std::endl;
}

void test_deep_scope_chain() {
    SymbolTable global;
    global.define("a", 1);

    SymbolTable level1(&global);
    SymbolTable level2(&level1);
    SymbolTable level3(&level2);

    assert(std::get<int>(level3.get("a")) == 1);
    std::cout << "test_deep_scope_chain passed" << std::endl;
}
void test_shadow_assign_behavior() {
    SymbolTable global;
    global.define("x", 10);

    SymbolTable local(&global);
    local.define("x", 20);
    local.assign("x", 30);

    assert(std::get<int>(local.get("x")) == 30);
    assert(std::get<int>(global.get("x")) == 10);
    std::cout << "test_shadow_assign_behavior passed" << std::endl;
}

void test_multiple_symbols() {
    SymbolTable st;
    st.define("a", 1);
    st.define("b", 2);
    st.define("c", 3);

    assert(std::get<int>(st.get("a")) == 1);
    assert(std::get<int>(st.get("b")) == 2);
    assert(std::get<int>(st.get("c")) == 3);
    std::cout << "test_multiple_symbols passed" << std::endl;
}

void test_shadowing() {
    SymbolTable global;
    global.define("x", 10);

    SymbolTable local(&global);
    local.define("x", 20);

    assert(std::get<int>(local.get("x")) == 20);
    assert(std::get<int>(global.get("x")) == 10);
    std::cout << "test_shadowing passed" << std::endl;
}

void test_assign() {
    SymbolTable st;
    st.define("x", 10);
    st.assign("x", 20);

    assert(std::get<int>(st.get("x")) == 20);
    std::cout << "test_assign passed" << std::endl;
}

void test_assign_parent() {
    SymbolTable global;
    global.define("x", 10);

    SymbolTable local(&global);
    local.assign("x", 20);

    assert(std::get<int>(global.get("x")) == 20);
    assert(std::get<int>(local.get("x")) == 20);
    std::cout << "test_assign_parent passed" << std::endl;
}

void test_undefined_variable() {
    SymbolTable st;
    try {
        st.get("z");
        assert(false);
    } catch (const std::runtime_error& e) {
    }
    std::cout << "test_undefined_variable passed" << std::endl;
}

void test_assign_undefined() {
    SymbolTable st;
    try {
        st.assign("z", 10);
        assert(false); 
    } catch (const std::runtime_error& e) {
    }
    std::cout << "test_assign_undefined passed" << std::endl;
}

int main() {
    test_basic_define_and_get();
    test_scope();
    test_deep_scope_chain();
    test_shadow_assign_behavior();
    test_multiple_symbols();
    test_shadowing();
    test_assign();
    test_assign_parent();
    test_undefined_variable();
    test_assign_undefined();

    std::cout << "All symbol table tests passed!" << std::endl;
    return 0;
}
