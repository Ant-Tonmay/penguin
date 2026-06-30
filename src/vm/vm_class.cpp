#include "vm/vm.h"

#include "vm/utils/access_utils.h"
#include "vm/utils/value_utils.h"

#include <iostream>

namespace vm {

bool VM::handleClassOp(CallFrame& frame, uint8_t instruction) {
    switch (instruction) {
        case OP_CLASS: {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            ClassObject* klass = allocate<ClassObject>(name);
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
                throwPenguinException(
                    "TypeError",
                    "Cannot define method '" + name + "': expected a class object but got '" + typeOf(klassValue) + "'."
                );
                return true;
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
            
            // Class shared fields
            if (std::holds_alternative<ClassObject*>(objectValue)) {
                ClassObject* klass = std::get<ClassObject*>(objectValue);
                if (klass->sharedFields.count(name)) {
                    push(klass->sharedFields[name]);
                    return true;
                }
                throwPenguinException(
                    "NameError",
                    "Class '" +
                    klass->name +
                    "' has no shared property '" +
                    name +
                    "'."
                );
                return true;
            }

            // Module exports

            if (std::holds_alternative<ModuleObject*>(objectValue))
            {
                ModuleObject* module =
                    std::get<ModuleObject*>(objectValue);

                auto it =
                    module->exports.find(name);

                if (it == module->exports.end())
                {
                    throwPenguinException("NameError","Module '" + module->name + "' has no export '" + name + "'" );
                    return true;
                }

                push(it->second);
                return true;
            }

            // Invalid receiver
            if (!std::holds_alternative<InstanceObject*>(objectValue)) {
                throwPenguinException(
                    "TypeError",
                    "Cannot access property '" +
                    name +
                    "' on value '" +
                    valueToString(objectValue) +
                    "'."
                );
                return true;
            }

            InstanceObject* instance = std::get<InstanceObject*>(objectValue);
            ClassObject* contextClass = frame.function->isMethod ? frame.function->ownerClass : nullptr;

            return lookupMember(
                instance,
                instance->klass,
                contextClass,
                name);
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
                throwPenguinException("NameError", "Class '" + klass->name + "' has no shared property '" + name + "'.");
                return true;
            }

            if (!std::holds_alternative<InstanceObject*>(objectValue)) {
                throwPenguinException("TypeError", "Cannot set property '" + name + "' on non-instance value.");
                return true;
            }

            InstanceObject* instance = std::get<InstanceObject*>(objectValue);
            ClassObject* contextClass = frame.function->isMethod ? frame.function->ownerClass : nullptr;
            
            if (instance->klass->sharedFields.count(name)) {
                instance->klass->sharedFields[name] = value;
                push(value);
                return true;
            }

            if (instance->klass->fields.count(name)) {
                AccessModifier access = instance->klass->fields[name];
                if (!checkAccess(instance->klass, contextClass, access)) {
                    throwPenguinException(
                        "RuntimeException",
                        "Access denied to field '" + name + "': field is not accessible from this context."
                    );

                    return true;
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

                if (instance->klass->sharedFields.count(name)) {
                    push(instance->klass->sharedFields[name]);
                    return true;
                }

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

            push(frame.function->module->globals[name]);
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

                if (instance->klass->sharedFields.count(name)) {
                    instance->klass->sharedFields[name] = value;
                    push(value);
                    return true;
                }

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

            frame.function->module->globals[name]= value;
            push(value);
            return true;
        }

        case OP_INHERIT: {
            Value superValue = pop();
            if (!std::holds_alternative<ClassObject*>(superValue)) {
                throwPenguinException("TypeError", "Superclass must be a class.");
                return true;
            }
            ClassObject* superclass = std::get<ClassObject*>(superValue);

            Value subclassValue = stack.back();
            ClassObject* subclass = std::get<ClassObject*>(subclassValue);
            subclass->parent = superclass;

            for (const auto& methodPair : superclass->methods) {
                if (superclass->methodAccess[methodPair.first] == AccessModifier::PRIVATE) {
                    continue;
                }
                auto& overloads = subclass->methods[methodPair.first];
                for (FunctionObject* fn : methodPair.second) {
                    bool exists = false;

                    for (FunctionObject* existing : overloads) {
                        if (existing->arity == fn->arity) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists)
                        overloads.push_back(fn);
                }
                subclass->methodAccess[methodPair.first] = superclass->methodAccess[methodPair.first];
            }
            for (const auto& fieldPair : superclass->fields) {
                if (fieldPair.second == AccessModifier::PRIVATE) {
                    continue;
                }
                if (!subclass->fields.count(fieldPair.first)) {
                    subclass->fields.emplace(fieldPair.first, fieldPair.second);
                }
            }
            // Copy shared fields as well
            for (const auto& sharedPair : superclass->sharedFields) {
                if (!subclass->sharedFields.count(sharedPair.first)) {
                    subclass->sharedFields.emplace(sharedPair.first, sharedPair.second);
                }
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
