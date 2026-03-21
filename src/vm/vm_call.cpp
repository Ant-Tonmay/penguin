#include "vm/vm.h"

#include <iostream>

namespace vm {

bool VM::handleCall(CallFrame& frame) {
    uint8_t argCount = frame.function->chunk.code[frame.ip++];
    Value calleeValue = stack[stack.size() - argCount - 1];

    if (std::holds_alternative<ClassObject*>(calleeValue)) {
        ClassObject* klass = std::get<ClassObject*>(calleeValue);
        
        if (klass->isTrait) {
            std::cerr << "Runtime error: Cannot instantiate trait '" << klass->name << "' directly." << std::endl;
            return false;
        }

        InstanceObject* instance = new InstanceObject(klass);

        if (klass->methods.count(klass->name)) {
            auto& initMethods = klass->methods[klass->name];
            FunctionObject* matchingInit = nullptr;
            for (auto* func : initMethods) {
                if (argCount == func->arity - 1) {
                    matchingInit = func;
                    break;
                }
            }

            if (matchingInit) {
                stack[stack.size() - argCount - 1] = instance;
                size_t base = stack.size() - argCount - 1;
                frames.push_back({matchingInit, 0, base});
                return true;
            }

            std::cerr << "Runtime error: no matching constructor for "
                      << static_cast<int>(argCount) << " arguments." << std::endl;
            return false;
        }

        if (argCount != 0) {
            std::cerr << "Runtime error: expected 0 arguments for default constructor but got "
                      << static_cast<int>(argCount) << std::endl;
            return false;
        }

        for (int i = 0; i < argCount; i++) pop();
        pop();
        push(instance);
        return true;
    }

    if (std::holds_alternative<BoundMethod*>(calleeValue)) {
        BoundMethod* bound = std::get<BoundMethod*>(calleeValue);
        stack[stack.size() - argCount - 1] = bound->instance;

        FunctionObject* matchingMethod = nullptr;
        for (auto* func : bound->methods) {
            if (argCount == func->arity - 1) {
                matchingMethod = func;
                break;
            }
        }

        if (!matchingMethod) {
            std::cerr << "Runtime error: expected method arguments did not match any overloaded method." << std::endl;
            return false;
        }

        size_t base = stack.size() - argCount - 1;
        frames.push_back({matchingMethod, 0, base});
        return true;
    }

    if (!std::holds_alternative<FunctionObject*>(calleeValue)) {
        std::cerr << "Runtime error: tried to call a non-function" << std::endl;
        return false;
    }

    FunctionObject* callee = std::get<FunctionObject*>(calleeValue);
    if (argCount != callee->arity) {
        std::cerr << "Runtime error: in function " << callee->name
                  << " expected " << callee->arity << " arguments but got "
                  << static_cast<int>(argCount) << std::endl;
        return false;
    }

    size_t base = stack.size() - argCount - 1;
    frames.push_back({callee, 0, base});
    return true;
}

bool VM::handleReturn(CallFrame& frame) {
    Value result = pop();
    size_t base = frame.base;

    frames.pop_back();
    if (frames.empty()) {
        return false;
    }

    stack.resize(base);
    push(result);
    return true;
}

}  // namespace vm
