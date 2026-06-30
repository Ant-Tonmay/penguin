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

struct ExceptionHandler {
    size_t frameIndex;
    size_t catchJumpOffset;
    size_t stackSize;
};

class VM {
public:

    ObjHeader* objects = nullptr;

    ~VM();

    template<typename T, typename... Args>
    T* allocate(Args&&... args);

    template<typename T>
    void trackObject(T* obj);

    std::vector<Value> stack;
    std::vector<CallFrame> frames;
    // std::unordered_map<std::string, Value> globals;
    std::vector<ExceptionHandler> exceptionHandlers;
    Value currentException;
    bool hasPendingReturn = false;
    Value pendingReturnValue;


    ModuleObject* builtinsModule = nullptr;

    std::unordered_map<
        std::string,
        ModuleObject*
    > loadedModules;

    VM();
    void push(Value v);
    Value pop();

    void run(FunctionObject* script,const std::vector<FunctionObject*>& compiledFunctions);
    void throwRuntimeError(const std::string& message);
    Value deepCopyIfNeeded(const Value& value);

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
    void registerBuiltins();
    void throwPenguinException(const std::string& className, const std::string& message);
    // ClassObject* resolveExceptionClass(const std::string& className);
    
    ModuleObject* loadModule(const std::string& importerFile,const std::string& moduleName);
    void executeModule(FunctionObject* script);

    bool lookupMember(InstanceObject* receiver,ClassObject* lookupClass,ClassObject* contextClass,const std::string& name);
};

    template<typename T>
    void VM::trackObject(T* obj)
    {
        if (!obj) return;

        // already tracked?
        if (obj->next != nullptr || objects == obj)
            return;

        obj->next = objects;
        objects = obj;
    }

    template<typename T, typename... Args>
    T* VM::allocate(Args&&... args)
    {
        T* obj = new T(std::forward<Args>(args)...);
        trackObject(obj);
        return obj;
    }

}
