#include "vm/vm.h"

#include "vm/utils/value_utils.h"

#include <iostream>

namespace vm {

void VM::push(Value value) {
    stack.push_back(value);
}

Value VM::pop() {
    Value value = stack.back();
    stack.pop_back();
    return value;
}

void VM::run(FunctionObject* script) {
    frames.push_back({script, 0, 0});

    while (true) {
        auto& frame = frames.back();
        uint8_t instruction = frame.function->chunk.code[frame.ip++];
        if (!executeInstruction(frame, instruction)) {
            return;
        }
    }
}

bool VM::executeInstruction(CallFrame& frame, uint8_t instruction) {
    switch (instruction) {
        case OP_CONSTANT: {
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
        case OP_GET_LOCAL: {
            uint8_t slot = frame.function->chunk.code[frame.ip++];
            push(stack[frame.base + slot]);
            return true;
        }
        case OP_SET_LOCAL: {
            uint8_t slot = frame.function->chunk.code[frame.ip++];
            stack[frame.base + slot] = stack.back();
            return true;
        }
        case OP_GET_GLOBAL: {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            push(globals[name]);
            return true;
        }
        case OP_SET_GLOBAL: {
            uint8_t nameIdx = frame.function->chunk.code[frame.ip++];
            std::string name = std::get<std::string>(frame.function->chunk.constants[nameIdx]);
            globals[name] = stack.back();
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

        case OP_PRINT: {
            Value value = pop();
            std::cout << valueToString(value);
            return true;
        }
        case OP_PRINTLN: {
            Value value = pop();
            std::cout << valueToString(value) << std::endl;
            return true;
        }

        case OP_CALL:
            return handleCall(frame);
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

        case OP_HALT:
            return false;

        default:
            std::cerr << "Runtime error: unknown opcode " << static_cast<int>(instruction) << std::endl;
            return false;
    }
}

}  // namespace vm
