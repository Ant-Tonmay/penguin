#pragma once

#include <variant>
#include <string>
#include <stdexcept>
#include <iostream>
#include "parser/ast.h"
#include <unordered_map>

struct FunctionObject;
struct ArrayObject; 
struct ObjectObject;
struct ClassObject;
struct InstanceObject;
struct BoundMethod;
struct Environment;

struct EnvReference {
    Environment* env;
    std::string name;
};


using Value = std::variant<
    int,      
    bool,
    char,
    float,
    double,
    long,
    long long,
    ArrayObject*,
    std::monostate,
    FunctionObject*,
    ObjectObject*,
    ClassObject*,
    InstanceObject*,
    BoundMethod*,
    EnvReference*,
    std::string 
>;

struct ObjectObject {
    std::unordered_map<std::string, Value> fields;
};

struct ArrayObject {
    bool isFixed;
    size_t length;
    size_t capacity;
    Value* data;      // Dynamic array of Values
    int refCount;     // Simple ref counting if we want to manage memory manually

    ArrayObject() : isFixed(false), length(0), capacity(0), data(nullptr), refCount(0) {}
    ~ArrayObject() {
        if (data) delete[] data;
    }
};
struct FunctionObject {
    Function* astNode; 
};
struct BoundMethod {
    InstanceObject* instance;
    std::string methodName;
};
