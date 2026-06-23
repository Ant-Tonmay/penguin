#include "vm/vm.h"

#include "vm/utils/value_utils.h"

#include <iostream>
#include "exceptions/error.h"
#include "interpreter/runtime_value.h"

namespace vm
{

    void VM::throwRuntimeError(const std::string &message)
    {
        if (frames.empty())
        {
            throw RuntimeError(message, {"", 0, 0});
        }
        auto &frame = frames.back();
        SourceLocation loc = {"", 0, 0};
        size_t idx = frame.ip > 0 ? frame.ip - 1 : 0;
        if (!frame.function->chunk.locations.empty() && idx < frame.function->chunk.locations.size())
        {
            loc = frame.function->chunk.locations[idx];
        }

        if (!exceptionHandlers.empty())
        {
            throw RuntimeError(message, loc); // VM::run will catch it and jump
        }

        throw RuntimeError(message, loc);
    }
    void VM::throwPenguinException(const std::string &className, const std::string &message)
    {
        // Look up the built-in class from globals
        
        auto& globals = builtinsModule->globals;

        auto it = globals.find(className);

        if (it == globals.end())
        {
            throw RuntimeError(
                "Unknown builtin exception: " +
                className,
                {}
            );
        }

        ClassObject* klass =
            std::get<ClassObject*>(it->second);

        // Create an instance and set .message
        InstanceObject *inst = new InstanceObject(klass);
        inst->fields["message"] = message;

        // Put the source location in a .location field as a string
        auto &frame = frames.back();
        size_t idx = frame.ip > 0 ? frame.ip - 1 : 0;
        if (!frame.function->chunk.locations.empty() && idx < frame.function->chunk.locations.size())
        {
            SourceLocation loc = frame.function->chunk.locations[idx];
            inst->fields["source"] = loc.line;
            inst->fields["line"] = static_cast<int64_t>(loc.line_num);
            inst->fields["column"] = static_cast<int64_t>(loc.col_num);
        }

        // Simulate OP_THROW: push and jump to handler, or rethrow
        if (!exceptionHandlers.empty())
        {
            auto handler = exceptionHandlers.back();
            exceptionHandlers.pop_back();
            while (frames.size() > handler.frameIndex + 1)
                frames.pop_back();
            stack.resize(handler.stackSize);
            push(inst);
            frames.back().ip = handler.catchJumpOffset;
            // Signal to the caller to restart the dispatch loop
            throw PenguinThrow{}; // ← a new internal signal (see 2.2)
        }
        else
        {
            // Unhandled — format and throw to C++ top level
            throw RuntimeError("[" + className + "] " + message, {});
        }
    }

    void VM::push(Value value)
    {
        stack.push_back(value);
    }

    Value VM::pop()
    {
        Value value = stack.back();
        stack.pop_back();
        return value;
    }

    void VM::run(FunctionObject *script)
    {   

         builtinsModule = new ModuleObject();
        builtinsModule->name = "__builtins__";
        if (!script->module) {
            auto* mainModule = new ModuleObject();
            mainModule->name = "main";
            script->module = mainModule;
        }
        registerBuiltins();
        frames.push_back({script, 0, 0});

        while (true)
        {
            auto &frame = frames.back();
            uint8_t instruction = frame.function->chunk.code[frame.ip++];

            try
            {
                if (!executeInstruction(frame, instruction))
                {
                    return;
                }
            }
            catch (const PenguinThrow&)
            {
                continue;  // Handler was already set up by throwPenguinException
            }
            catch (const RuntimeError &e)
            {
                if (!exceptionHandlers.empty())
                {
                    auto handler = exceptionHandlers.back();
                    exceptionHandlers.pop_back();

                    while (frames.size() > handler.frameIndex + 1)
                    {
                        frames.pop_back();
                    }

                    stack.resize(handler.stackSize);
                    push(e.message);

                    frames.back().ip = handler.catchJumpOffset;
                    continue;
                }
                else
                {
                    throw; // Rethrow to top level
                }
            }
        }
    }

    bool VM::executeInstruction(CallFrame &frame, uint8_t instruction)
    {
        switch (instruction)
        {
        case OP_CONSTANT:
        {
            uint8_t idx = frame.function->chunk.code[frame.ip++];
            push(frame.function->chunk.constants[idx]);
            return true;
        }
        case OP_TRUE:
            push(true);
            return true;
        case OP_FALSE:
            push(false);
            return true;
        case OP_NULL:
            push(std::monostate{});
            return true;
        case OP_GET_LOCAL:
        {
            uint8_t slot = frame.function->chunk.code[frame.ip++];
            Value val = stack[frame.base + slot];
            if (std::holds_alternative<ReferenceObject *>(val))
            {
                ReferenceObject *ref = std::get<ReferenceObject *>(val);
                if (ref->type == ReferenceObject::Type::LOCAL)
                {
                    push(stack[ref->stackIndex]);
                }
                else if (ref->type == ReferenceObject::Type::GLOBAL)
                {
                    push(frame.function->module->globals[ref->name]);
                }
            }
            else
            {
                push(val);
            }
            return true;
        }
        case OP_SET_LOCAL:
        {
            uint8_t slot = frame.function->chunk.code[frame.ip++];
            Value &target = stack[frame.base + slot];
            if (std::holds_alternative<ReferenceObject *>(target))
            {
                ReferenceObject *ref = std::get<ReferenceObject *>(target);
                if (ref->type == ReferenceObject::Type::LOCAL)
                {
                    stack[ref->stackIndex] = stack.back();
                }
                else if (ref->type == ReferenceObject::Type::GLOBAL)
                {
                    frame.function->module->globals[ref->name] = stack.back();
                }
            }
            else
            {
                target = stack.back();
            }
            return true;
        }
        case OP_GET_GLOBAL:
        {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            auto& globals = frame.function->module->globals;
            auto it = globals.find(name);
            Value val;
            if (it != globals.end())
            {
                val = it->second;
            }
            else if (builtinsModule)
            {
                auto bit = builtinsModule->globals.find(name);
                if (bit != builtinsModule->globals.end())
                {
                    val = bit->second;
                }
            }
            if (std::holds_alternative<ReferenceObject *>(val))
            {
                ReferenceObject *ref = std::get<ReferenceObject *>(val);
                if (ref->type == ReferenceObject::Type::LOCAL)
                {
                    push(stack[ref->stackIndex]);
                }
                else if (ref->type == ReferenceObject::Type::GLOBAL)
                {
                    push(globals[ref->name]);
                }
            }
            else
            {
                push(val);
            }
            return true;
        }
        case OP_SET_GLOBAL:
        {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            auto& globals = frame.function->module->globals;
            Value &target = globals[name];
            if (std::holds_alternative<ReferenceObject *>(target))
            {
                ReferenceObject *ref = std::get<ReferenceObject *>(target);
                if (ref->type == ReferenceObject::Type::LOCAL)
                {
                    stack[ref->stackIndex] = stack.back();
                }
                else if (ref->type == ReferenceObject::Type::GLOBAL)
                {
                    globals[ref->name] = stack.back();
                }
            }
            else{
                target = stack.back();
            }
            return true;
        }
        case OP_POP:
            pop();
            return true;

        case OP_ADD:
        case OP_PLUS_EQUAL:
        case OP_SUB:
        case OP_MINUS_EQUAL:
        case OP_MUL:
        case OP_MULTIPLY_EQUAL:
        case OP_DIV:
        case OP_DIVIDE_EQUAL:
        case OP_MOD:
        case OP_MODULO_EQUAL:
        case OP_BITWISE_AND:
        case OP_BITWISE_AND_EQUAL:
        case OP_BITWISE_OR:
        case OP_BITWISE_OR_EQUAL:
        case OP_XOR:
        case OP_XOR_EQUAL:
        case OP_LEFT_SHIFT:
        case OP_LEFT_SHIFT_EQUAL:
        case OP_RIGHT_SHIFT:
        case OP_RIGHT_SHIFT_EQUAL:
        case OP_LOGICAL_AND:
        case OP_LOGICAL_AND_EQUAL:
        case OP_LOGICAL_OR:
        case OP_LOGICAL_OR_EQUAL:
            return handleArithmetic(instruction);

        case OP_GREATER:
        case OP_LESSER:
        case OP_GREATER_EQUAL:
        case OP_LESSER_EQUAL:
        case OP_EQUAL:
        case OP_NOT_EQUAL:
        case OP_NOT:
        case OP_NEGATE:
            return handleComparison(instruction);

        case OP_JUMP:
        case OP_JUMP_IF_FALSE:
        case OP_LOOP:
            return handleJump(frame, instruction);

        case OP_PRINT:
        {
            Value value = pop();
            std::cout << valueToString(value);
            return true;
        }
        case OP_PRINTLN:
        {
            Value value = pop();
            std::cout << valueToString(value) << std::endl;
            return true;
        }

        case OP_CALL:
            return handleCall(frame);

        case OP_TRY_BEGIN:
        {
            uint16_t offset = (frame.function->chunk.code[frame.ip] << 8) | frame.function->chunk.code[frame.ip + 1];
            frame.ip += 2;
            exceptionHandlers.push_back({frames.size() - 1,
                                         frame.ip + offset,
                                         stack.size()});
            return true;
        }
        case OP_TRY_END:
        {
            if (!exceptionHandlers.empty())
            {
                exceptionHandlers.pop_back();
            }
            return true;
        }
        case OP_MATCH_TYPE:
        {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string typeName = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            Value exc = stack.back();
            bool matches = false;
            if (typeName == "Any")
            {
                // "Any" catches everything
                matches = true;
            }
            else if (std::holds_alternative<InstanceObject *>(exc))
            {
                // Walk the class inheritance chain
                InstanceObject *inst = std::get<InstanceObject *>(exc);
                ClassObject *klass = inst->klass;
                while (klass)
                {
                    if (klass->name == typeName)
                    {
                        matches = true;
                        break;
                    }
                    klass = klass->parent;
                }
            }
            else if (std::holds_alternative<std::string>(exc) && typeName == "String")
            {
                matches = true;
        auto& globals =
    frame.function->module->globals;    }
            else if (std::holds_alternative<int64_t>(exc) && typeName == "Int")
            {
                matches = true;
            }
            else if (std::holds_alternative<double>(exc) && typeName == "Float")
            {
                matches = true;
            }
            else if (std::holds_alternative<bool>(exc) && typeName == "Bool")
            {
                matches = true;
            }
            else if (std::holds_alternative<char>(exc) && typeName == "Char")
            {
                matches = true;
            }
            else if (std::holds_alternative<std::monostate>(exc) && typeName == "Null")
            {
                matches = true;
            }
            push(matches);
            return true;
        }
        case OP_DEEP_COPY:
        {
            push(deepCopyIfNeeded(pop()));
            return true;
        }
        case OP_MAKE_REF_LOCAL:
        {
            uint8_t slot = frame.function->chunk.code[frame.ip++];
            ReferenceObject *ref = new ReferenceObject();
            ref->type = ReferenceObject::Type::LOCAL;
            ref->stackIndex = frame.base + slot;
            push(ref);
            return true;
        }
        case OP_MAKE_REF_GLOBAL:
        {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            ReferenceObject *ref = new ReferenceObject();
            ref->type = ReferenceObject::Type::GLOBAL;
            ref->name = name;
            push(ref);
            return true;
        }
        case OP_THROW:
        {
            currentException = pop();

            if (!std::holds_alternative<InstanceObject*>(currentException))
            {
                throwPenguinException(
                    "TypeError",
                    "Cannot throw value '" +
                    valueToString(currentException) +
                    "'. Only exception objects can be thrown."
                );
                return true;
            }

            InstanceObject* instance =
            std::get<InstanceObject*>(currentException);

            // Must inherit from Exception.
            bool isException = false;

            for (ClassObject* klass = instance->klass;
                klass != nullptr;
                klass = klass->parent)
            {
                if (klass->name == "Exception")
                {
                    isException = true;
                    break;
                }
            }

            if (!isException)
            {
                throwPenguinException(
                    "TypeError",
                    "Only objects derived from Exception can be thrown."
                );
                return true;
            }

            if (exceptionHandlers.empty())
            {
                throwRuntimeError("Unhandled exception: " + valueToString(currentException));
                return false;
            }

            auto handler = exceptionHandlers.back();
            exceptionHandlers.pop_back();

            while (frames.size() > handler.frameIndex + 1)
            {
                frames.pop_back();
            }

            stack.resize(handler.stackSize);
            push(currentException);

            frames.back().ip = handler.catchJumpOffset;
            return true;
        }
        case OP_RETURN:
            return handleReturn(frame);

        case OP_NEW_ARRAY:
        case OP_INDEX_GET:
        case OP_INDEX_SET:
        case OP_FIXED_ARRAY:
        case OP_ARRAY_PUSH:
        case OP_ARRAY_LENGTH:
            return handleArrayOp(frame, instruction);

        case OP_CLASS:
        case OP_METHOD:
        case OP_GET_PROPERTY:
        case OP_SET_PROPERTY:
        case OP_GET_PROPERTY_OR_GLOBAL:
        case OP_SET_PROPERTY_OR_LOCAL:
        case OP_INHERIT:
        case OP_FIELD:
            return handleClassOp(frame, instruction);

        case OP_TRAIT:
            return handleTraitOp(frame, instruction);

        case OP_CAST_INT:
        case OP_CAST_FLOAT:
        case OP_CAST_STRING:
        case OP_CAST_BOOL:
        case OP_CAST_CHAR:
        case OP_TYPEOF:
            return handleCastOp(instruction);
        case OP_READLINE:
            return handleInputOp(instruction);

        case OP_SAVE_RETURN:
        {
            pendingReturnValue = pop();
            hasPendingReturn = true;
            return true;
        }
        case OP_RESTORE_RETURN:
        {
            if (hasPendingReturn)
            {
                hasPendingReturn = false;
                push(pendingReturnValue);
                return handleReturn(frames.back());
            }
            return true;
        }

        case OP_HALT:
            return false;

        default:
            std::cerr << "Runtime error: unknown opcode " << static_cast<int>(instruction) << std::endl;
            return false;
        }
    }

} // namespace vm
