#pragma once
#include "chunk.h"
#include "../parser/ast.h"
#include <vector>

namespace vm {

struct Local {
    std::string name;
    int depth;
};

struct LoopContext {
    int loopStart;                    // offset to jump back to for 'continue' (while loops)
    std::vector<int> breakJumps;      // pending forward jumps for 'break'
    std::vector<int> continueJumps;   // pending forward jumps for 'continue' (for loops)
};

// Tracks a finally block during compilation so that `return` inside try
// can emit OP_SAVE_RETURN + OP_JUMP and register the jump for later patching.
struct FinallyContext {
    std::vector<int>* pendingJumps;   // Pointer to the jumpsToFinally vector owned by compileTryCatchStmt
};

struct ClassCompiler {
    std::string className;
    std::string parentName;
    ClassCompiler* enclosing = nullptr;
};

class Compiler {
public:
    FunctionObject* currentFunction;              // function being compiled right now
    std::vector<FunctionObject*> compiledFunctions; // all compiled non-main functions
    std::vector<Local> locals;
    int scopeDepth = 0;
    std::vector<LoopContext> loopStack;
    SourceLocation currentLocation{"", 0, 0};
    std::unordered_map<std::string, std::vector<Param>> functionSignatures;

    FunctionObject* compile(ASTNode* node);

private:
    ClassCompiler* currentClass = nullptr;
    Chunk& currentChunk();

    void emit(uint8_t byte);
    void emitConstant(Value v);

    int emitJump(uint8_t instruction);
    void patchJump(int offset);
    void emitLoop(int loopStart);

    void beginScope();
    void endScope();
    void addLocal(const std::string& name);
    int resolveLocal(const std::string& name);
    
    // Stack of active finally contexts for return-inside-try handling.
    // When a return is compiled inside a try-with-finally, it emits
    // OP_SAVE_RETURN + OP_JUMP and records the jump offset here.
    std::vector<FinallyContext> activeFinallyContexts;

    void compileFunction(Function* func);
    void compileExpr(ASTNode*);
    void compileStmt(ASTNode*);
    void compileTryCatchStmt(TryCatchStmt* stmt);
};

}
