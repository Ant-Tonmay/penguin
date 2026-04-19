#include "vm/compiler.h"

namespace vm {

Chunk& Compiler::currentChunk() {
    return currentFunction->chunk;
}

void Compiler::emit(uint8_t byte) {
    currentChunk().write(byte, currentLocation);
}

void Compiler::emitConstant(Value value) {
    int idx = currentChunk().addConstant(value);
    emit(OP_CONSTANT);
    emit(idx);
}

int Compiler::emitJump(uint8_t instruction) {
    emit(instruction);
    emit(0xff);
    emit(0xff);
    return currentChunk().code.size() - 2;
}

void Compiler::patchJump(int offset) {
    int jump = currentChunk().code.size() - offset - 2;
    if (jump > UINT16_MAX) {
        // Technically this should throw a compiler error.
    }
    currentChunk().code[offset] = (jump >> 8) & 0xff;
    currentChunk().code[offset + 1] = jump & 0xff;
    // Note: locations remain unchanged during patch.
}

void Compiler::emitLoop(int loopStart) {
    emit(OP_LOOP);
    int offset = currentChunk().code.size() - loopStart + 2;
    if (offset > UINT16_MAX) {
        // Technically this should throw a compiler error.
    }
    emit((offset >> 8) & 0xff);
    emit(offset & 0xff);
}

void Compiler::beginScope() {
    scopeDepth++;
}

void Compiler::endScope() {
    scopeDepth--;
    while (!locals.empty() && locals.back().depth > scopeDepth) {
        emit(OP_POP);
        locals.pop_back();
    }
}

void Compiler::addLocal(const std::string& name) {
    locals.push_back({name, scopeDepth});
}

int Compiler::resolveLocal(const std::string& name) {
    for (int i = static_cast<int>(locals.size()) - 1; i >= 0; i--) {
        if (locals[i].name == name) {
            return i;
        }
    }
    return -1;
}

void Compiler::compileFunction(Function* func) {
    auto* fnObj = new FunctionObject(func->name, func->params.size());

    FunctionObject* enclosingFunction = currentFunction;
    std::vector<Local> enclosingLocals = std::move(locals);
    int enclosingScopeDepth = scopeDepth;

    currentFunction = fnObj;
    locals.clear();
    scopeDepth = 0;

    beginScope();
    addLocal("");  // Reserve slot 0 for callee.

    for (const auto& param : func->params) {
        addLocal(param.name);
    }

    for (const auto& stmt : func->body->statements) {
        compileStmt(stmt.get());
    }

    emit(OP_NULL);
    emit(OP_RETURN);

    currentFunction = enclosingFunction;
    locals = std::move(enclosingLocals);
    scopeDepth = enclosingScopeDepth;

    compiledFunctions.push_back(fnObj);
}

FunctionObject* Compiler::compile(ASTNode* node) {
    if (auto* program = dynamic_cast<Program*>(node)) {
        for (const auto& func : program->functions) {
            if (func->name != "main") {
                compileFunction(func.get());
            }
        }

        auto* scriptFn = new FunctionObject("__script__", 0);
        currentFunction = scriptFn;
        locals.clear();
        scopeDepth = 0;

        for(const auto& trait : program->traits){
            compileStmt(trait.get());
        }
        for (const auto& cls : program->classes) {
            compileStmt(cls.get());
        }

        for (const auto& func : program->functions) {
            if (func->name == "main") {
                beginScope();
                for (const auto& stmt : func->body->statements) {
                    compileStmt(stmt.get());
                }
                endScope();
                break;
            }
        }

        emit(OP_HALT);
        return scriptFn;
    }

    auto* scriptFn = new FunctionObject("__script__", 0);
    currentFunction = scriptFn;
    compileStmt(node);
    emit(OP_RETURN);
    return scriptFn;
}

}  // namespace vm
