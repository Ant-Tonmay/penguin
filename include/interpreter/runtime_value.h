#pragma once

#include <variant>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "parser/ast.h"
#include "symbol_table/value.h"

struct ClassObject {
    std::string name;
    ClassObject* parent;
    std::unordered_map<std::string, std::vector<MethodDef*>> methods;
    std::unordered_map<std::string, AccessModifier> methodAccess;

    std::unordered_map<std::string, AccessModifier> fields;
};

struct InstanceObject {
    ClassObject* klass;
    std::unordered_map<std::string, Value> fieldValues;

    InstanceObject(ClassObject* k) : klass(k) {}
};

struct ModuleObject {
    std::string name;

    std::unordered_map<std::string, Value> globals;
    std::unordered_map<std::string, Value> exports;
};




inline std::string valueToString(const Value& val) {
    if (std::holds_alternative<int>(val)) {
        return std::to_string(std::get<int>(val));
    } else if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? "true" : "false";
    } else if (std::holds_alternative<std::string>(val)) {
        return std::get<std::string>(val);
    } else if (std::holds_alternative<double>(val)) {
        return std::to_string(std::get<double>(val));
    } else if (std::holds_alternative<float>(val)) {
        return std::to_string(std::get<float>(val));
    } else if (std::holds_alternative<long>(val)) {
        return std::to_string(std::get<long>(val));
    } else if (std::holds_alternative<long long>(val)) {
        return std::to_string(std::get<long long>(val));
    } else if (std::holds_alternative<char>(val)) {
        return std::string(1, std::get<char>(val));
    } else if (std::holds_alternative<ArrayObject*>(val)) {
        auto arr = std::get<ArrayObject*>(val);
        return "[Array length=" + std::to_string(arr->length) + "]";
    } else if (std::holds_alternative<std::monostate>(val)) {
        return "null";
    }
    return "";
}

inline void printValue(const Value& val) {
    std::cout << valueToString(val);
}

