#pragma once

#include <variant>
#include <string>
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "opcode.h"
#include "../parser/ast.h"

namespace vm {

// Forward declarations
struct FunctionObject;
struct ArrayObject; 
struct ObjectObject;
struct ClassObject;
struct InstanceObject;
struct BoundMethod;


using Value = std::variant<
    int64_t,      
    bool,
    char,
    double,
    __int128,
    ArrayObject*,
    std::monostate,
    FunctionObject*,
    ObjectObject*,
    ClassObject*,
    InstanceObject*,
    BoundMethod*,
    std::string 
>;

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::vector<SourceLocation> locations;

    void write(uint8_t byte, SourceLocation loc = {"", 0, 0}) {
        code.push_back(byte);
        locations.push_back(loc);
    }

    void write16(uint16_t value, SourceLocation loc = {"", 0, 0}) {
        code.push_back((value >> 8) & 0xff);
        locations.push_back(loc);
        code.push_back(value & 0xff);
        locations.push_back(loc);
    }

    int addConstant(Value v) {
        constants.push_back(v);
        return constants.size() - 1;
    }
};

struct ObjectObject {
    std::unordered_map<std::string, Value> fields;
};

struct ClassObject {
    std::string name;
    bool isTrait = false;
    ClassObject* parent = nullptr;
    std::unordered_map<std::string, std::vector<FunctionObject*>> methods;
    std::unordered_map<std::string, AccessModifier> methodAccess;
    std::unordered_map<std::string, AccessModifier> fields;
    std::unordered_map<std::string, Value> sharedFields;
    
    ClassObject(const std::string& name) : name(name) {}
};

struct InstanceObject {
    ClassObject* klass;
    std::unordered_map<std::string, Value> fields;
    
    InstanceObject(ClassObject* klass) : klass(klass) {}
};

struct ArrayObject {
    bool isFixed;
    size_t length;
    size_t capacity;
    Value* data;    
    int refCount;    

    ArrayObject() : isFixed(false), length(0), capacity(0), data(nullptr), refCount(0) {}
    ~ArrayObject() {
        // if (data) delete[] data;
    }
};

struct FunctionObject {
    std::string name;
    int arity;
    bool isMethod;
    ClassObject* ownerClass = nullptr;
    Chunk chunk;

    FunctionObject(const std::string& name, int arity, bool isMethod = false)
        : name(name), arity(arity), isMethod(isMethod) {}
};

struct BoundMethod {
    InstanceObject* instance;
    std::vector<FunctionObject*> methods;

    BoundMethod(InstanceObject* instance, std::vector<FunctionObject*> methods)
        : instance(instance), methods(std::move(methods)) {}
};

}
