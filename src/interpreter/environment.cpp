#include "interpreter/environment.h"

Environment::Environment(Environment* parent) : parent(parent) {}

void Environment::define(const std::string& name, const Value& value) {
    values[name] = value;
}

void Environment::assign(const std::string& name, const Value& value) {
    if (values.count(name)) {
        if (std::holds_alternative<EnvReference*>(values[name])) {
            EnvReference* ref = std::get<EnvReference*>(values[name]);
            ref->env->assign(ref->name, value);
        } else {
            values[name] = value;
        }
        return;
    }

    if (parent) {
        parent->assign(name, value);
        return;
    }

    throw std::runtime_error("Undefined variable '" + name + "'.");
}

Value Environment::get(const std::string& name) {
    if (values.count(name)) {
        if (std::holds_alternative<EnvReference*>(values[name])) {
            EnvReference* ref = std::get<EnvReference*>(values[name]);
            return ref->env->get(ref->name);
        }
        return values[name];
    }

    if (parent) {
        return parent->get(name);
    }

    throw std::runtime_error("Undefined variable '" + name + "'.");
}

EnvReference* Environment::getReference(const std::string& name) {
    if (values.count(name)) {
        if (std::holds_alternative<EnvReference*>(values[name])) {
            return std::get<EnvReference*>(values[name]); // return the underlying reference
        }
        EnvReference* ref = new EnvReference();
        ref->env = this;
        ref->name = name;
        return ref;
    }

    if (parent) {
        return parent->getReference(name);
    }

    throw std::runtime_error("Undefined variable '" + name + "'.");
}
