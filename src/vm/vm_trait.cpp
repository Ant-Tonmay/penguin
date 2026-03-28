#include "vm/vm.h"

#include "vm/utils/access_utils.h"
#include "vm/utils/value_utils.h"

#include <iostream>

namespace vm {

bool VM::handleTraitOp(CallFrame& frame, uint8_t instruction) {
    switch (instruction) {
        case OP_TRAIT: {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            ClassObject* klass = new ClassObject(name);
            klass->isTrait = true;
            push(klass);
            return true;
        }

        case OP_METHOD: {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            uint8_t modifier = frame.function->chunk.code[frame.ip++];

            Value methodValue = pop();
            Value klassValue = stack.back();
            if (!std::holds_alternative<ClassObject*>(klassValue)) {
                std::cerr << "Runtime error: OP_METHOD expects class on stack." << std::endl;
                return false;
            }

            ClassObject* klass = std::get<ClassObject*>(klassValue);
            FunctionObject* func = std::get<FunctionObject*>(methodValue);
            func->ownerClass = klass;

            bool overridden = false;
            for (auto& existingMethod : klass->methods[name]) {
                if (existingMethod->arity == func->arity) {
                    existingMethod = func;
                    overridden = true;
                    break;
                }
            }
            if (!overridden) {
                klass->methods[name].push_back(func);
            }

            klass->methodAccess[name] = static_cast<AccessModifier>(modifier);
            return true;
        }

        case OP_GET_PROPERTY: {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            Value objectValue = pop();
            
            if (std::holds_alternative<ClassObject*>(objectValue)) {
                ClassObject* klass = std::get<ClassObject*>(objectValue);
                if (klass->sharedFields.count(name)) {
                    push(klass->sharedFields[name]);
                    return true;
                }
                std::cerr << "Runtime error: Undefined shared property '" << name << "'." << std::endl;
                return false;
            }

            if (!std::holds_alternative<InstanceObject*>(objectValue)) {
                std::cerr << "Runtime error: OP_GET_PROPERTY expects an instance or class. Got: "
                          << valueToString(objectValue) << std::endl;
                return false;
            }

            InstanceObject* instance = std::get<InstanceObject*>(objectValue);
            ClassObject* contextClass = frame.function->isMethod ? frame.function->ownerClass : nullptr;

            if (instance->fields.count(name) || instance->klass->fields.count(name)) {
                AccessModifier access = AccessModifier::PUBLIC;
                if (instance->klass->fields.count(name)) {
                    access = instance->klass->fields[name];
                }
                if (!checkAccess(instance->klass, contextClass, access)) {
                    std::cerr << "Runtime error: Access denied to field '" << name << "'." << std::endl;
                    return false;
                }
                if (instance->fields.count(name)) {
                    push(instance->fields[name]);
                } else {
                    push(std::monostate{});
                }
                return true;
            }

            if (instance->klass->methods.count(name)) {
                AccessModifier access = instance->klass->methodAccess[name];
                if (!checkAccess(instance->klass, contextClass, access)) {
                    std::cerr << "Runtime error: Access denied to method '" << name << "'." << std::endl;
                    return false;
                }

                std::vector<FunctionObject*> methods = instance->klass->methods[name];
                BoundMethod* bound = new BoundMethod(instance, methods);
                push(bound);
                return true;
            }

            std::cerr << "Runtime error: Undefined property '" << name << "'." << std::endl;
            return false;
        }

        case OP_SET_PROPERTY: {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            Value value = pop();
            Value objectValue = pop();

            if (std::holds_alternative<ClassObject*>(objectValue)) {
                ClassObject* klass = std::get<ClassObject*>(objectValue);
                if (klass->sharedFields.count(name)) {
                    klass->sharedFields[name] = value;
                    push(value);
                    return true;
                }
                std::cerr << "Runtime error: Undefined shared property '" << name << "'." << std::endl;
                return false;
            }

            if (!std::holds_alternative<InstanceObject*>(objectValue)) {
                std::cerr << "Runtime error: OP_SET_PROPERTY expects an instance or class." << std::endl;
                return false;
            }

            InstanceObject* instance = std::get<InstanceObject*>(objectValue);
            ClassObject* contextClass = frame.function->isMethod ? frame.function->ownerClass : nullptr;
            if (instance->klass->fields.count(name)) {
                AccessModifier access = instance->klass->fields[name];
                if (!checkAccess(instance->klass, contextClass, access)) {
                    std::cerr << "Runtime error: Access denied to field '" << name << "'." << std::endl;
                    return false;
                }
            }

            instance->fields[name] = value;
            push(value);
            return true;
        }

        case OP_GET_PROPERTY_OR_GLOBAL: {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            Value objectValue = pop();

            if (std::holds_alternative<InstanceObject*>(objectValue)) {
                InstanceObject* instance = std::get<InstanceObject*>(objectValue);
                ClassObject* contextClass = frame.function->isMethod ? frame.function->ownerClass : nullptr;

                if (instance->fields.count(name) || instance->klass->fields.count(name)) {
                    AccessModifier access = AccessModifier::PUBLIC;
                    if (instance->klass->fields.count(name)) {
                        access = instance->klass->fields[name];
                    }
                    if (checkAccess(instance->klass, contextClass, access)) {
                        if (instance->fields.count(name)) {
                            push(instance->fields[name]);
                        } else {
                            push(std::monostate{});
                        }
                        return true;
                    }
                } else if (instance->klass->methods.count(name)) {
                    AccessModifier access = instance->klass->methodAccess[name];
                    if (checkAccess(instance->klass, contextClass, access)) {
                        std::vector<FunctionObject*> methods = instance->klass->methods[name];
                        BoundMethod* bound = new BoundMethod(instance, methods);
                        push(bound);
                        return true;
                    }
                }
            }

            push(globals[name]);
            return true;
        }

        case OP_SET_PROPERTY_OR_LOCAL: {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            Value objectValue = pop();
            Value value = pop();

            if (std::holds_alternative<InstanceObject*>(objectValue)) {
                InstanceObject* instance = std::get<InstanceObject*>(objectValue);
                ClassObject* contextClass = frame.function->isMethod ? frame.function->ownerClass : nullptr;

                bool allowed = true;
                if (instance->klass->fields.count(name)) {
                    AccessModifier access = instance->klass->fields[name];
                    allowed = checkAccess(instance->klass, contextClass, access);
                }
                if (allowed) {
                    instance->fields[name] = value;
                    push(value);
                    return true;
                }
            }

            globals[name] = value;
            push(value);
            return true;
        }

        case OP_INHERIT: {
            Value superValue = pop();
            if (!std::holds_alternative<ClassObject*>(superValue)) {
                std::cerr << "Runtime error: Superclass must be a class." << std::endl;
                return false;
            }
            ClassObject* superclass = std::get<ClassObject*>(superValue);

            Value subclassValue = stack.back();
            ClassObject* subclass = std::get<ClassObject*>(subclassValue);
            subclass->parent = superclass;

            for (const auto& methodPair : superclass->methods) {
                if (superclass->methodAccess[methodPair.first] == AccessModifier::PRIVATE) {
                    continue;
                }
                subclass->methods[methodPair.first] = methodPair.second;
                subclass->methodAccess[methodPair.first] = superclass->methodAccess[methodPair.first];
            }
            for (const auto& fieldPair : superclass->fields) {
                if (fieldPair.second == AccessModifier::PRIVATE) {
                    continue;
                }
                subclass->fields[fieldPair.first] = fieldPair.second;
            }
            // Copy shared fields as well
            for (const auto& sharedPair : superclass->sharedFields) {
                // if access modifier is private, we should skip, but sharedFields doesn't store modifiers directly in the same map.
                // assuming shared fields are accessible
                subclass->sharedFields[sharedPair.first] = sharedPair.second;
            }
            return true;
        }

        case OP_FIELD: {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            uint8_t modifier = frame.function->chunk.code[frame.ip++];

            Value klassValue = stack.back();
            ClassObject* klass = std::get<ClassObject*>(klassValue);
            
            if (modifier == static_cast<uint8_t>(AccessModifier::SHARED)) {
                klass->sharedFields[name] = std::monostate{};
            } else {
                klass->fields[name] = static_cast<AccessModifier>(modifier);
            }
            return true;
        }

        default:
            return false;
    }
}

}  // namespace vm
