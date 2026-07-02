#include "vm/vm.h"

#include "vm/utils/value_utils.h"
#include <iostream>

namespace vm {

bool VM::handleArithmetic(uint8_t instruction) {
    switch (instruction) {
        case OP_ADD:
        case OP_PLUS_EQUAL: {
            Value b = pop();
            Value a = pop();
            if (std::holds_alternative<std::string>(a) || std::holds_alternative<std::string>(b)) {
                push(valueToString(a) + valueToString(b));
            } else if (std::holds_alternative<int64_t>(a) && std::holds_alternative<int64_t>(b)) {
                push(std::get<int64_t>(a) + std::get<int64_t>(b));
            } else {
                push(asDouble(a) + asDouble(b));
            }
            return true;
        }
        case OP_SUB:
        case OP_MINUS_EQUAL: {
            Value b = pop();
            Value a = pop();
            if (std::holds_alternative<int64_t>(a) && std::holds_alternative<int64_t>(b)) {
                push(std::get<int64_t>(a) - std::get<int64_t>(b));
            } else {
                push(asDouble(a) - asDouble(b));
            }
            return true;
        }
        case OP_MUL:
        case OP_MULTIPLY_EQUAL: {
            Value b = pop();
            Value a = pop();
            if (std::holds_alternative<int64_t>(a) && std::holds_alternative<int64_t>(b)) {
                push(std::get<int64_t>(a) * std::get<int64_t>(b));
            } else {
                push(asDouble(a) * asDouble(b));
            }
            return true;
        }
        case OP_DIV:
        case OP_DIVIDE_EQUAL: {
            double b = asDouble(pop());
            double a = asDouble(pop());
            if (b == 0) {
                throwBuiltinException("DivisionByZeroException","Cannot divide by zero.");
            }
            push(a / b);
            return true;
        }
        case OP_MOD:
        case OP_MODULO_EQUAL: {
            int64_t b = asInt(pop());
            int64_t a = asInt(pop());
            push(static_cast<int64_t>(a % b));
            return true;
        }
        case OP_BITWISE_AND:
        case OP_BITWISE_AND_EQUAL: {
            int b = asInt(pop());
            int a = asInt(pop());
            push(static_cast<double>(a & b));
            return true;
        }
        case OP_BITWISE_OR:
        case OP_BITWISE_OR_EQUAL: {
            int b = asInt(pop());
            int a = asInt(pop());
            push(static_cast<double>(a | b));
            return true;
        }
        case OP_XOR:
        case OP_XOR_EQUAL: {
            int b = asInt(pop());
            int a = asInt(pop());
            push(static_cast<double>(a ^ b));
            return true;
        }
        case OP_LEFT_SHIFT:
        case OP_LEFT_SHIFT_EQUAL: {
            int b = asInt(pop());
            int a = asInt(pop());
            push(static_cast<double>(a << b));
            return true;
        }
        case OP_RIGHT_SHIFT:
        case OP_RIGHT_SHIFT_EQUAL: {
            int b = asInt(pop());
            int a = asInt(pop());
            push(static_cast<double>(a >> b));
            return true;
        }
        case OP_LOGICAL_AND:
        case OP_LOGICAL_AND_EQUAL: {
            bool b = asBool(pop());
            bool a = asBool(pop());
            push(a && b);
            return true;
        }
        case OP_LOGICAL_OR:
        case OP_LOGICAL_OR_EQUAL: {
            bool b = asBool(pop());
            bool a = asBool(pop());
            push(a || b);
            return true;
        }
        default:
            return false;
    }
}

bool VM::handleComparison(uint8_t instruction) {
    switch (instruction) {
        case OP_GREATER: {
            double b = asDouble(pop());
            double a = asDouble(pop());
            push(a > b);
            return true;
        }
        case OP_LESSER: {
            double b = asDouble(pop());
            double a = asDouble(pop());
            push(a < b);
            return true;
        }
        case OP_GREATER_EQUAL: {
            double b = asDouble(pop());
            double a = asDouble(pop());
            push(a >= b);
            return true;
        }
        case OP_LESSER_EQUAL: {
            double b = asDouble(pop());
            double a = asDouble(pop());
            push(a <= b);
            return true;
        }
        case OP_EQUAL: {
            double b = asDouble(pop());
            double a = asDouble(pop());
            push(a == b);
            return true;
        }
        case OP_NOT_EQUAL: {
            double b = asDouble(pop());
            double a = asDouble(pop());
            push(a != b);
            return true;
        }
        case OP_NOT: {
            bool value = asBool(pop());
            push(!value);
            return true;
        }
        case OP_NEGATE: {
            double value = asDouble(pop());
            push(-value);
            return true;
        }
        default:
            return false;
    }
}

bool VM::handleJump(CallFrame& frame, uint8_t instruction) {
    switch (instruction) {
        case OP_JUMP: {
            uint16_t offset =
                (frame.function->chunk.code[frame.ip] << 8) |
                frame.function->chunk.code[frame.ip + 1];
            frame.ip += 2;
            frame.ip += offset;
            return true;
        }
        case OP_JUMP_IF_FALSE: {
            uint16_t offset =
                (frame.function->chunk.code[frame.ip] << 8) |
                frame.function->chunk.code[frame.ip + 1];
            frame.ip += 2;
            if (!asBool(stack.back())) {
                frame.ip += offset;
            }
            return true;
        }
        case OP_LOOP: {
            uint16_t offset =
                (frame.function->chunk.code[frame.ip] << 8) |
                frame.function->chunk.code[frame.ip + 1];
            frame.ip += 2;
            frame.ip -= offset;
            return true;
        }
        default:
            return false;
    }
}

bool VM::handleCastOp(uint8_t instruction) {
    Value value = pop();

    switch (instruction) {
        case OP_CAST_INT:
            if (std::holds_alternative<std::string>(value)) push(Value(static_cast<int64_t>(std::stoll(std::get<std::string>(value)))));
            else if (std::holds_alternative<double>(value)) push(Value(static_cast<int64_t>(std::get<double>(value))));
            else if (std::holds_alternative<bool>(value)) push(Value(static_cast<int64_t>(std::get<bool>(value))));
            else if (std::holds_alternative<char>(value)) push(Value(static_cast<int64_t>(std::get<char>(value))));
            else push(value);
            return true;

        case OP_CAST_FLOAT:
            if (std::holds_alternative<std::string>(value)) push(Value(static_cast<double>(std::stod(std::get<std::string>(value)))));
            else if (std::holds_alternative<int64_t>(value)) push(Value(static_cast<double>(std::get<int64_t>(value))));
            else if (std::holds_alternative<bool>(value)) push(Value(static_cast<double>(std::get<bool>(value))));
            else if (std::holds_alternative<char>(value)) push(Value(static_cast<double>(std::get<char>(value))));
            else push(value);
            return true;

        case OP_CAST_STRING:
            push(Value(valueToString(value)));
            return true;

        case OP_CAST_BOOL:
            if (std::holds_alternative<int64_t>(value)) push(Value(static_cast<bool>(std::get<int64_t>(value))));
            else if (std::holds_alternative<double>(value)) push(Value(static_cast<bool>(std::get<double>(value))));
            else if (std::holds_alternative<std::string>(value)) push(Value(!std::get<std::string>(value).empty()));
            else if (std::holds_alternative<char>(value)) push(Value(static_cast<bool>(std::get<char>(value))));
            else push(value);
            return true;

        case OP_CAST_CHAR:
            if (std::holds_alternative<int64_t>(value)) push(Value(static_cast<char>(std::get<int64_t>(value))));
            else if (std::holds_alternative<double>(value)) push(Value(static_cast<char>(std::get<double>(value))));
            else if (std::holds_alternative<std::string>(value)) {
                const std::string str = std::get<std::string>(value);
                push(Value(static_cast<char>(str.empty() ? 0 : str[0])));
            } else if (std::holds_alternative<bool>(value)) {
                push(Value(static_cast<char>(std::get<bool>(value))));
            } else {
                push(value);
            }
            return true;

        case OP_TYPEOF:
            push(typeOf(value));
            return true;

        default:
            push(value);
            return false;
    }
}

bool VM::handleInputOp(uint8_t instruction) {
    switch (instruction) {
        case OP_READLINE: {
            std::string line;
            std::getline(std::cin, line);
            push(line);
            return true;
        }
        default:
            return false;
    }
}

}  // namespace vm
