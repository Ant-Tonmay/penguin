#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <memory>
#include "interpreter/runtime_value.h"

class Environment {
public:
    Environment(Environment* parent = nullptr);
    ~Environment() = default;

    void define(const std::string& name, const Value& value);
    void assign(const std::string& name, const Value& value);
    Value get(const std::string& name);
    EnvReference* getReference(const std::string& name);
    
    Environment* getParent() const { return parent; }

private:
    Environment* parent;
    std::unordered_map<std::string, Value> values;
};
