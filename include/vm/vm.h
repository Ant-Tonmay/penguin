#pragma once
#include "chunk.h"
#include <vector>
#include <unordered_map>

namespace vm {

struct CallFrame {
    FunctionObject* function;
    size_t ip;
    size_t base;  // stack base for this call frame
};

class VM {
public:
    std::vector<Value> stack;
    std::vector<CallFrame> frames;
    std::unordered_map<std::string, Value> globals;

    void push(Value v);
    Value pop();

    void run(FunctionObject* script);
    void throwRuntimeError(const std::string& message);

private:
    bool executeInstruction(CallFrame& frame, uint8_t instruction);
    bool handleArithmetic(uint8_t instruction);
    bool handleComparison(uint8_t instruction);
    bool handleJump(CallFrame& frame, uint8_t instruction);
    bool handleCall(CallFrame& frame);
    bool handleReturn(CallFrame& frame);
    bool handleArrayOp(CallFrame& frame, uint8_t instruction);
    bool handleClassOp(CallFrame& frame, uint8_t instruction);
    bool handleTraitOp(CallFrame& frame, uint8_t instruction);
    bool handleCastOp(uint8_t instruction);
    bool handleInputOp(uint8_t instruction);
};

}
