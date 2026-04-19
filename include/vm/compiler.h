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

class Compiler {
public:
    FunctionObject* currentFunction;              // function being compiled right now
    std::vector<FunctionObject*> compiledFunctions; // all compiled non-main functions
    std::vector<Local> locals;
    int scopeDepth = 0;
    std::vector<LoopContext> loopStack;
    SourceLocation currentLocation{"", 0, 0};

    FunctionObject* compile(ASTNode* node);

private:
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

    void compileFunction(Function* func);
    void compileExpr(ASTNode*);
    void compileStmt(ASTNode*);
};

}
