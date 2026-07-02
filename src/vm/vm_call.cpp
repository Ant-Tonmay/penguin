#include "vm/vm.h"

namespace vm {

bool VM::handleCall(CallFrame& frame) {
    uint8_t argCount = frame.function->chunk.code[frame.ip++];
    Value calleeValue = stack[stack.size() - argCount - 1];

    if (std::holds_alternative<ClassObject*>(calleeValue)) {
        ClassObject* klass = std::get<ClassObject*>(calleeValue);

        if (klass->isTrait) {
            throwBuiltinException("TypeError", "Cannot instantiate trait '" + klass->name + "' directly.");
            return true;
        }

        InstanceObject* instance = allocate<InstanceObject>(klass);

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

            throwBuiltinException("TypeError", "No matching constructor for class '" + klass->name + "' with " + std::to_string(argCount) + " arguments.");
            return true;
        }

        if (argCount != 0) {
            throwBuiltinException("TypeError", "Expected 0 arguments for default constructor of '" + klass->name + "' but got " + std::to_string(argCount) + ".");
            return true;
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
            throwBuiltinException("TypeError", "No matching overload for bound method with " + std::to_string(argCount) + " arguments.");
            return true;
        }

        size_t base = stack.size() - argCount - 1;
        frames.push_back({matchingMethod, 0, base});
        return true;
    }

    if (!std::holds_alternative<FunctionObject*>(calleeValue)) {
        throwBuiltinException("TypeError", "Tried to call a non-function value.");
        return true;
    }

    FunctionObject* callee = std::get<FunctionObject*>(calleeValue);
    if (argCount != callee->arity) {
        throwBuiltinException("TypeError", "Function '" + callee->name + "' expected " + std::to_string(callee->arity) + " arguments but got " + std::to_string(argCount) + ".");
        return true;
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
