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

enum class ObjType {
    CLASS,
    INSTANCE,
    FUNCTION,
    BOUND_METHOD,
    ARRAY,
    REFERENCE,
    MODULE,
    OBJECT
};

struct ObjHeader {
    ObjType type;
    ObjHeader* next = nullptr;

    virtual ~ObjHeader() = default;
};

// Forward declarations
struct FunctionObject;
struct ArrayObject; 
struct ObjectObject;
struct ClassObject;
struct InstanceObject;
struct BoundMethod;
struct ReferenceObject;
struct ModuleObject;


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
    ReferenceObject*,
    ModuleObject*,
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

struct ObjectObject : ObjHeader{
    std::unordered_map<std::string, Value> fields;

    ObjectObject() {
        type = ObjType::OBJECT;
    }
};

struct ClassObject : ObjHeader {
    std::string name;
    bool isTrait = false;
    ClassObject* parent = nullptr;
    std::unordered_map<std::string, std::vector<FunctionObject*>> methods;
    std::unordered_map<std::string, AccessModifier> methodAccess;
    std::unordered_map<std::string, AccessModifier> fields;
    std::unordered_map<std::string, Value> sharedFields;
    
    ClassObject(const std::string& name)
        : name(name)
    {
        type = ObjType::CLASS;
    }
};

struct InstanceObject : ObjHeader {
    ClassObject* klass;
    std::unordered_map<std::string, Value> fields;
    
     InstanceObject(ClassObject* klass)
        : klass(klass)
    {
        type = ObjType::INSTANCE;
    }
};

struct ArrayObject : ObjHeader {
    bool isFixed;
    size_t length;
    size_t capacity;
    Value* data;    
    int refCount;    

    ArrayObject()
        : isFixed(false),
          length(0),
          capacity(0),
          data(nullptr),
          refCount(0)
    {
        type = ObjType::ARRAY;
    }

    ~ArrayObject() {
        delete[] data;
    }
};

struct ModuleObject : ObjHeader {
    std::string name;
    std::string filePath;

    std::unordered_map<std::string, Value> globals;
    std::unordered_map<std::string, Value> exports;

    ModuleObject() {
        type = ObjType::MODULE;
    }
};

struct FunctionObject : ObjHeader {
    std::string name;
    int arity;
    bool isMethod;
    ClassObject* ownerClass = nullptr;
    Chunk chunk;
    ModuleObject* module = nullptr;

    FunctionObject(const std::string& name, int arity, bool isMethod = false)
        : name(name),
          arity(arity),
          isMethod(isMethod)
    {
        type = ObjType::FUNCTION;
    }
};

struct BoundMethod : ObjHeader {
    InstanceObject* instance;
    std::vector<FunctionObject*> methods;

    BoundMethod(
        InstanceObject* instance,
        std::vector<FunctionObject*> methods)
        : instance(instance),
          methods(std::move(methods))
    {
        type = ObjType::BOUND_METHOD;
    }
};

struct ReferenceObject : ObjHeader{
    enum class Type { LOCAL, GLOBAL };
    Type type;
    int stackIndex;       // For LOCAL
    std::string name;     // For GLOBAL

    ModuleObject* module = nullptr;
    ReferenceObject() {
        this->type = Type::LOCAL;
        ObjHeader::type = ObjType::REFERENCE;
    }
};

}


