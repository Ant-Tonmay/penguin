#include <iostream>
#include <vector>
#include <cassert>
#include "vm/vm.h"
#include "vm/chunk.h"
#include "vm/opcode.h"
#include "vm/value.h"

// Helper to create a simple chunk
// void test_basic_arithmetic() {
//     std::cout << "Testing Basic Arithmetic..." << std::endl;
    
//     vm::FunctionObject fn("test", 0);
//     vm::VM vm;

//     // 1.2 + 3.4
//     int c1 = fn.chunk.addConstant(1.2);
//     int c2 = fn.chunk.addConstant(3.4);

//     fn.chunk.write(vm::OP_CONSTANT);
//     fn.chunk.write(c1);

//     fn.chunk.write(vm::OP_CONSTANT);
//     fn.chunk.write(c2);

//     fn.chunk.write(vm::OP_ADD);
//     fn.chunk.write(vm::OP_PRINT);
//     fn.chunk.write(vm::OP_HALT);

//     vm.run(&fn);
    
//     std::cout << "Basic Arithmetic Test Passed (Clean Run)" << std::endl;
// }

// void test_classes() {
//     std::cout << "Testing Classes and Objects..." << std::endl;
    
//     vm::FunctionObject fn("test", 0);
//     vm::VM vm;

//     // 1. Define class "Point"
//     int classNameIdx = fn.chunk.addConstant(std::string("Point"));
//     fn.chunk.write(vm::OP_CLASS);
//     fn.chunk.write(classNameIdx);
    
//     // Save class to global to keep it around
//     fn.chunk.write(vm::OP_SET_GLOBAL);
//     fn.chunk.write(classNameIdx);
//     fn.chunk.write(vm::OP_POP); // Keep stack clean

//     // 2. Instantiate "Point" 
//     fn.chunk.write(vm::OP_GET_GLOBAL);
//     fn.chunk.write(classNameIdx);
    
//     // Call the class (0 arguments) to instantiate
//     fn.chunk.write(vm::OP_CALL);
//     fn.chunk.write(0);
    
//     // 3. Set property "x" = 10 on the instance
//     int xIdx = fn.chunk.addConstant(std::string("x"));
    
//     // Duplicate instance on stack so we can use it again
//     // We don't have OP_DUP, so let's save instance to global "p"
//     int pIdx = fn.chunk.addConstant(std::string("p"));
//     fn.chunk.write(vm::OP_SET_GLOBAL);
//     fn.chunk.write(pIdx);
//     fn.chunk.write(vm::OP_POP);
    
//     // Get "p"
//     fn.chunk.write(vm::OP_GET_GLOBAL);
//     fn.chunk.write(pIdx);
    
//     // Value 10
//     int val10 = fn.chunk.addConstant((int64_t)10);
//     fn.chunk.write(vm::OP_CONSTANT);
//     fn.chunk.write(val10);
    
//     // p.x = 10
//     fn.chunk.write(vm::OP_SET_PROPERTY);
//     fn.chunk.write(xIdx);
//     fn.chunk.write(vm::OP_POP); // pop assignment result
    
//     // 4. Get property "x"
//     fn.chunk.write(vm::OP_GET_GLOBAL);
//     fn.chunk.write(pIdx);
//     fn.chunk.write(vm::OP_GET_PROPERTY);
//     fn.chunk.write(xIdx);
    
//     // print
//     fn.chunk.write(vm::OP_PRINTLN);
    
//     fn.chunk.write(vm::OP_HALT);

//     vm.run(&fn);
    
//     std::cout << "Class and Object Test Passed (Clean Run)" << std::endl;
// }

int main() {
    //test_basic_arithmetic();
    //test_classes();
    return 0;
}

