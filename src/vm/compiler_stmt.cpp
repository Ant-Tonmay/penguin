#include "exceptions/error.h"
#include "vm/compiler.h"

namespace vm {

void Compiler::compileStmt(ASTNode* node) {
    if (node) currentLocation = node->loc;
    if (auto* printStmt = dynamic_cast<PrintStmt*>(node)) {
        compileExpr(printStmt->expression.get());
        emit(OP_PRINT);
    } else if (auto* printlnStmt = dynamic_cast<PrintlnStmt*>(node)) {
        compileExpr(printlnStmt->expression.get());
        emit(OP_PRINTLN);
    } else if (auto* exprStmt = dynamic_cast<ExprStmt*>(node)) {
        compileExpr(exprStmt->expression.get());
        emit(OP_POP);
    } else if (auto* returnStmt = dynamic_cast<ReturnStmt*>(node)) {
        if (returnStmt->value) {
            compileExpr(returnStmt->value.get());
        } else {
            emit(OP_NULL);
        }
        
        for (auto it = activeFinallyBlocks.rbegin(); it != activeFinallyBlocks.rend(); ++it) {
            compileStmt(*it);
        }
        
        emit(OP_RETURN);
    } else if (auto* throwStmt = dynamic_cast<ThrowStmt*>(node)) {
        compileExpr(throwStmt->expression.get());
        emit(OP_THROW);
    } else if (auto* ifStmt = dynamic_cast<IfStmt*>(node)) {
        compileExpr(ifStmt->condition.get());
        int thenJump = emitJump(OP_JUMP_IF_FALSE);
        emit(OP_POP);

        compileStmt(ifStmt->thenBranch.get());

        int elseJump = emitJump(OP_JUMP);
        patchJump(thenJump);
        emit(OP_POP);

        if (ifStmt->elseBranch) {
            compileStmt(ifStmt->elseBranch.get());
        }

        patchJump(elseJump);
    } else if (auto* whileStmt = dynamic_cast<WhileStmt*>(node)) {
        int loopStart = currentChunk().code.size();
        loopStack.push_back({loopStart, {}});

        compileExpr(whileStmt->condition.get());
        int exitJump = emitJump(OP_JUMP_IF_FALSE);
        emit(OP_POP);

        compileStmt(whileStmt->body.get());
        emitLoop(loopStart);

        patchJump(exitJump);
        emit(OP_POP);

        for (int breakJump : loopStack.back().breakJumps) {
            patchJump(breakJump);
        }
        loopStack.pop_back();
    } else if (auto* forStmt = dynamic_cast<ForStmt*>(node)) {
        beginScope();

        if (forStmt->init) {
            compileStmt(forStmt->init.get());
        }

        int loopStart = currentChunk().code.size();
        int exitJump = -1;

        if (forStmt->condition) {
            compileExpr(forStmt->condition.get());
            exitJump = emitJump(OP_JUMP_IF_FALSE);
            emit(OP_POP);
        }

        loopStack.push_back({-1, {}, {}});
        compileStmt(forStmt->body.get());

        for (int continueJump : loopStack.back().continueJumps) {
            patchJump(continueJump);
        }

        if (forStmt->increment) {
            compileStmt(forStmt->increment.get());
        }

        emitLoop(loopStart);

        if (exitJump != -1) {
            patchJump(exitJump);
            emit(OP_POP);
        }

        for (int breakJump : loopStack.back().breakJumps) {
            patchJump(breakJump);
        }
        loopStack.pop_back();

        endScope();
    } else if (auto* assignStmt = dynamic_cast<AssignmentStmt*>(node)) {
        for (const auto& assign : assignStmt->assignments) {
            if (auto* var = dynamic_cast<VarExpr*>(assign.target.get())) {
                int arg = resolveLocal(var->name);
                bool isLocal = true;
                bool isNewLocal = false;

                if (arg == -1 && scopeDepth > 0) {
                    int thisArg = resolveLocal("this");
                    if (thisArg != -1) {
                        Value nameVal = var->name;
                        int idx = currentChunk().addConstant(nameVal);

                        compileExpr(assign.value.get());

                        if (assign.op != TokenType::EQUAL) {
                            emit(OP_GET_LOCAL);
                            emit(thisArg);
                            emit(OP_GET_PROPERTY_OR_GLOBAL);
                            emit(idx);

                            switch (assign.op) {
                                case TokenType::PLUS_EQUAL: emit(OP_PLUS_EQUAL); break;
                                case TokenType::MINUS_EQUAL: emit(OP_MINUS_EQUAL); break;
                                case TokenType::STAR_EQUAL: emit(OP_MULTIPLY_EQUAL); break;
                                case TokenType::SLASH_EQUAL: emit(OP_DIVIDE_EQUAL); break;
                                case TokenType::MOD_OP_EQUAL: emit(OP_MODULO_EQUAL); break;
                                case TokenType::BITWISE_AND_EQUAL: emit(OP_BITWISE_AND_EQUAL); break;
                                case TokenType::BITWISE_OR_EQUAL: emit(OP_BITWISE_OR_EQUAL); break;
                                case TokenType::XOR_EQUAL: emit(OP_XOR_EQUAL); break;
                                default: break;
                            }
                        }

                        emit(OP_GET_LOCAL);
                        emit(thisArg);
                        emit(OP_SET_PROPERTY_OR_LOCAL);
                        emit(idx);
                        continue;
                    }

                    addLocal(var->name);
                    arg = locals.size() - 1;
                    isNewLocal = true;
                } else if (arg == -1 && scopeDepth == 0) {
                    isLocal = false;
                    Value nameVal = var->name;
                    arg = currentChunk().addConstant(nameVal);
                }

                if (assign.op != TokenType::EQUAL) {
                    if (isLocal) {
                        emit(OP_GET_LOCAL);
                        emit(arg);
                    } else {
                        emit(OP_GET_GLOBAL);
                        emit(arg);
                    }
                    compileExpr(assign.value.get());

                    switch (assign.op) {
                        case TokenType::PLUS_EQUAL: emit(OP_PLUS_EQUAL); break;
                        case TokenType::MINUS_EQUAL: emit(OP_MINUS_EQUAL); break;
                        case TokenType::STAR_EQUAL: emit(OP_MULTIPLY_EQUAL); break;
                        case TokenType::SLASH_EQUAL: emit(OP_DIVIDE_EQUAL); break;
                        case TokenType::MOD_OP_EQUAL: emit(OP_MODULO_EQUAL); break;
                        case TokenType::BITWISE_AND_EQUAL: emit(OP_BITWISE_AND_EQUAL); break;
                        case TokenType::BITWISE_OR_EQUAL: emit(OP_BITWISE_OR_EQUAL); break;
                        case TokenType::XOR_EQUAL: emit(OP_XOR_EQUAL); break;
                        default: break;
                    }
                } else {
                    compileExpr(assign.value.get());
                }

                if (isNewLocal) {
                    // No-op.
                } else if (isLocal) {
                    emit(OP_SET_LOCAL);
                    emit(arg);
                    emit(OP_POP);
                } else {
                    emit(OP_SET_GLOBAL);
                    emit(arg);
                    emit(OP_POP);
                }
            } else if (auto* idx = dynamic_cast<IndexExpr*>(assign.target.get())) {
                compileExpr(idx->array.get());
                compileExpr(idx->index.get());
                
                if (assign.op != TokenType::EQUAL) {
                    compileExpr(idx->array.get());
                    compileExpr(idx->index.get());
                    emit(OP_INDEX_GET);
                    compileExpr(assign.value.get());
                    switch (assign.op) {
                        case TokenType::PLUS_EQUAL: emit(OP_PLUS_EQUAL); break;
                        case TokenType::MINUS_EQUAL: emit(OP_MINUS_EQUAL); break;
                        case TokenType::STAR_EQUAL: emit(OP_MULTIPLY_EQUAL); break;
                        case TokenType::SLASH_EQUAL: emit(OP_DIVIDE_EQUAL); break;
                        case TokenType::MOD_OP_EQUAL: emit(OP_MODULO_EQUAL); break;
                        case TokenType::BITWISE_AND_EQUAL: emit(OP_BITWISE_AND_EQUAL); break;
                        case TokenType::BITWISE_OR_EQUAL: emit(OP_BITWISE_OR_EQUAL); break;
                        case TokenType::XOR_EQUAL: emit(OP_XOR_EQUAL); break;
                        default: break;
                    }
                } else {
                    compileExpr(assign.value.get());
                }
                
                emit(OP_INDEX_SET);
            } else if (auto* mem = dynamic_cast<MemberExpr*>(assign.target.get())) {
                compileExpr(mem->object.get());
                int nameIdx = currentChunk().addConstant(mem->name);

                if (assign.op != TokenType::EQUAL) {
                    compileExpr(mem->object.get());
                    emit(OP_GET_PROPERTY);
                    emit(nameIdx);
                    
                    compileExpr(assign.value.get());
                    switch (assign.op) {
                        case TokenType::PLUS_EQUAL: emit(OP_PLUS_EQUAL); break;
                        case TokenType::MINUS_EQUAL: emit(OP_MINUS_EQUAL); break;
                        case TokenType::STAR_EQUAL: emit(OP_MULTIPLY_EQUAL); break;
                        case TokenType::SLASH_EQUAL: emit(OP_DIVIDE_EQUAL); break;
                        case TokenType::MOD_OP_EQUAL: emit(OP_MODULO_EQUAL); break;
                        case TokenType::BITWISE_AND_EQUAL: emit(OP_BITWISE_AND_EQUAL); break;
                        case TokenType::BITWISE_OR_EQUAL: emit(OP_BITWISE_OR_EQUAL); break;
                        case TokenType::XOR_EQUAL: emit(OP_XOR_EQUAL); break;
                        default: break;
                    }
                } else {
                    compileExpr(assign.value.get());
                }

                emit(OP_SET_PROPERTY);
                emit(nameIdx);
                emit(OP_POP);
            }
        }
    } else if (auto* block = dynamic_cast<Block*>(node)) {
        beginScope();
        for (const auto& stmt : block->statements) {
            compileStmt(stmt.get());
        }
        endScope();
    } else if (dynamic_cast<BreakStmt*>(node)) {
        if (!loopStack.empty()) {
            int breakJump = emitJump(OP_JUMP);
            loopStack.back().breakJumps.push_back(breakJump);
        }
    } else if (dynamic_cast<ContinueStmt*>(node)) {
        if (!loopStack.empty()) {
            auto& loop = loopStack.back();
            if (loop.loopStart >= 0) {
                emitLoop(loop.loopStart);
            } else {
                int continueJump = emitJump(OP_JUMP);
                loop.continueJumps.push_back(continueJump);
            }
        }
    } else if (dynamic_cast<Expr*>(node)) {
        compileExpr(node);
        emit(OP_POP);
    } else if (auto* classStmt = dynamic_cast<ClassStmt*>(node)) {
        int nameIdx = currentChunk().addConstant(classStmt->name);
        emit(OP_CLASS);
        emit(nameIdx);

        int arg = resolveLocal(classStmt->name);
        bool isLocal = true;
        if (arg == -1 && scopeDepth > 0) {
            addLocal(classStmt->name);
            arg = locals.size() - 1;
        } else if (arg == -1 && scopeDepth == 0) {
            isLocal = false;
            arg = currentChunk().addConstant(classStmt->name);
        }

        if (isLocal) {
            emit(OP_SET_LOCAL);
            emit(arg);
        } else {
            emit(OP_SET_GLOBAL);
            emit(arg);
        }

        if (!classStmt->parentName.empty()) {
            int parentArg = resolveLocal(classStmt->parentName);
            if (parentArg != -1) {
                emit(OP_GET_LOCAL);
                emit(parentArg);
            } else {
                int parentIdx = currentChunk().addConstant(classStmt->parentName);
                emit(OP_GET_GLOBAL);
                emit(parentIdx);
            }
            emit(OP_INHERIT);
        }
        
        for (const auto& implName : classStmt->impls) {
            int implArg = resolveLocal(implName);
            if (implArg != -1) {
                emit(OP_GET_LOCAL);
                emit(implArg);
            } else {
                int implIdx = currentChunk().addConstant(implName);
                emit(OP_GET_GLOBAL);
                emit(implIdx);
            }
            emit(OP_INHERIT);
        }

        for (const auto& section : classStmt->sections) {
            for (const auto& member : section->members) {
                if (auto* field = dynamic_cast<FieldDecl*>(member.get())) {
                    int fieldNameIdx = currentChunk().addConstant(field->name);
                    emit(OP_FIELD);
                    emit(fieldNameIdx);
                    emit(static_cast<uint8_t>(section->modifier));

                    if (field->initializer) {
                        if (section->modifier == AccessModifier::SHARED) {
                            if (isLocal) {
                                emit(OP_GET_LOCAL);
                                emit(arg);
                            } else {
                                emit(OP_GET_GLOBAL);
                                emit(arg);
                            }
                            compileExpr(field->initializer.get());
                            emit(OP_SET_PROPERTY);
                            emit(fieldNameIdx);
                            emit(OP_POP);
                        } else {
                            throw CompileError("Instance field initializers are not yet supported. Please initialize them in the constructor.", currentLocation);
                        }
                    }
                } else if (auto* method = dynamic_cast<MethodDef*>(member.get())) {
                    Function func(method->name, method->params, std::make_unique<Block>());

                    auto* fnObj = new FunctionObject(method->name, method->params.size() + 1, true);
                    FunctionObject* enclosingFunction = currentFunction;
                    std::vector<Local> enclosingLocals = std::move(locals);
                    int enclosingScopeDepth = scopeDepth;

                    currentFunction = fnObj;
                    locals.clear();
                    scopeDepth = 0;

                    beginScope();
                    addLocal("this");
                    for (const auto& param : method->params) {
                        addLocal(param.name);
                    }

                    for (size_t i = 0; i < method->params.size(); ++i) {
                        if (!method->params[i].isRef) {
                            emit(OP_GET_LOCAL);
                            emit(i + 1); // 0 is reserved for 'this'
                            emit(OP_DEEP_COPY);
                            emit(OP_SET_LOCAL);
                            emit(i + 1);
                            emit(OP_POP);
                        }
                    }

                    for (const auto& stmt : method->body->statements) {
                        compileStmt(stmt.get());
                    }

                    if (method->name == classStmt->name) {
                        emit(OP_GET_LOCAL);
                        emit(0);
                    } else {
                        emit(OP_NULL);
                    }
                    emit(OP_RETURN);

                    currentFunction = enclosingFunction;
                    locals = std::move(enclosingLocals);
                    scopeDepth = enclosingScopeDepth;

                    compiledFunctions.push_back(fnObj);

                    emitConstant(fnObj);
                    int methodNameIdx = currentChunk().addConstant(method->name);
                    emit(OP_METHOD);
                    emit(methodNameIdx);
                    emit(static_cast<uint8_t>(section->modifier));
                }
            }
        }

        emit(OP_POP);
    } else if (auto* traitStmt = dynamic_cast<TraitStmt*>(node)) {
        int nameIdx = currentChunk().addConstant(traitStmt->name);
        emit(OP_TRAIT);
        emit(nameIdx);

        int arg = resolveLocal(traitStmt->name);
        bool isLocal = true;
        if (arg == -1 && scopeDepth > 0) {
            addLocal(traitStmt->name);
            arg = locals.size() - 1;
        } else if (arg == -1 && scopeDepth == 0) {
            isLocal = false;
            arg = currentChunk().addConstant(traitStmt->name);
        }

        if (isLocal) {
            emit(OP_SET_LOCAL);
            emit(arg);
        } else {
            emit(OP_SET_GLOBAL);
            emit(arg);
        }

        for (const auto& parentName : traitStmt->parents) {
            int parentArg = resolveLocal(parentName);
            if (parentArg != -1) {
                emit(OP_GET_LOCAL);
                emit(parentArg);
            } else {
                int parentIdx = currentChunk().addConstant(parentName);
                emit(OP_GET_GLOBAL);
                emit(parentIdx);
            }
            emit(OP_INHERIT);
        }

        for (const auto& section : traitStmt->sections) {
            for (const auto& member : section->members) {
                if (auto* field = dynamic_cast<TraitFieldDecl*>(member.get())) {
                    int fieldNameIdx = currentChunk().addConstant(field->name);
                    emit(OP_FIELD);
                    emit(fieldNameIdx);
                    
                    AccessModifier am = AccessModifier::PUBLIC;
                    if (section->modifier == TraitAccessModifier::SHARED) am = AccessModifier::SHARED;
                    else if (section->modifier == TraitAccessModifier::PROTECTED) am = AccessModifier::PROTECTED;
                    
                    emit(static_cast<uint8_t>(am));

                    if (field->initializer) {
                        if (am == AccessModifier::SHARED) {
                            if (isLocal) {
                                emit(OP_GET_LOCAL);
                                emit(arg);
                            } else {
                                emit(OP_GET_GLOBAL);
                                emit(arg);
                            }
                            compileExpr(field->initializer.get());
                            emit(OP_SET_PROPERTY);
                            emit(fieldNameIdx);
                            emit(OP_POP);
                        } else {
                            throw CompileError("Instance field initializers are not yet supported in traits.", currentLocation);
                        }
                    }
                } else if (auto* method = dynamic_cast<TraitMethodDecl*>(member.get())) {
                    auto* fnObj = new FunctionObject(method->name, method->params.size() + 1, true);
                    fnObj->chunk.write(OP_NULL);
                    fnObj->chunk.write(OP_RETURN);
                    compiledFunctions.push_back(fnObj);

                    emitConstant(fnObj);
                    int methodNameIdx = currentChunk().addConstant(method->name);
                    emit(OP_METHOD);
                    emit(methodNameIdx);
                    
                    AccessModifier am = AccessModifier::PUBLIC;
                    if (section->modifier == TraitAccessModifier::SHARED) am = AccessModifier::SHARED;
                    else if (section->modifier == TraitAccessModifier::PROTECTED) am = AccessModifier::PROTECTED;
                    
                    emit(static_cast<uint8_t>(am));
                }
            }
        }

        emit(OP_POP);
    } else if (auto* tryStmt = dynamic_cast<TryCatchStmt*>(node)) {
        compileTryCatchStmt(tryStmt);
    } else if (auto* aliasStmt = dynamic_cast<AliasStmt*>(node)) {
        if (aliasStmt->include) {
            // Create a ClassObject to act as a module namespace
            int nameIdx = currentChunk().addConstant(aliasStmt->name);
            emit(OP_CLASS);
            emit(nameIdx);

            // Store the class globally under the alias name
            int globalIdx = currentChunk().addConstant(aliasStmt->name);
            emit(OP_SET_GLOBAL);
            emit(globalIdx);

            // For each resolved export, register it as a SHARED field and
            // copy the global value into the class's sharedFields
            for (const auto& exportName : aliasStmt->resolvedExports) {
                int fieldNameIdx = currentChunk().addConstant(exportName);
                emit(OP_FIELD);
                emit(fieldNameIdx);
                emit(static_cast<uint8_t>(AccessModifier::SHARED));

                // Get the alias class back on the stack
                emit(OP_GET_GLOBAL);
                emit(globalIdx);

                // Get the exported global value
                int exportIdx = currentChunk().addConstant(exportName);
                emit(OP_GET_GLOBAL);
                emit(exportIdx);

                // Set it as a property on the class
                emit(OP_SET_PROPERTY);
                emit(fieldNameIdx);
                emit(OP_POP);
            }

            emit(OP_POP); // Pop the class from the stack
        } else {
            // Variable alias: alias newName = existingName;
            int existingIdx = currentChunk().addConstant(aliasStmt->vname);
            emit(OP_GET_GLOBAL);
            emit(existingIdx);

            int aliasIdx = currentChunk().addConstant(aliasStmt->name);
            emit(OP_SET_GLOBAL);
            emit(aliasIdx);
            emit(OP_POP);
        }
    }
}

void Compiler::compileTryCatchStmt(TryCatchStmt* stmt) {
    if (stmt->finallyBlock) {
        activeFinallyBlocks.push_back(stmt->finallyBlock.get());
    }

    int tryBeginJump = emitJump(OP_TRY_BEGIN);
    compileStmt(stmt->tryBlock.get());
    emit(OP_TRY_END);
    
    int skipCatchJump = emitJump(OP_JUMP);
    
    patchJump(tryBeginJump);
    
    std::vector<int> endJumps;

    for (auto& catchClause : stmt->catchBlocks) {
        int typeIdx = currentChunk().addConstant(catchClause.exceptionType);
        emit(OP_MATCH_TYPE);
        emit(typeIdx);
        
        int nextCatchJump = emitJump(OP_JUMP_IF_FALSE);
        emit(OP_POP); // Pop the 'matches' boolean
        
        beginScope();
        addLocal(catchClause.variableName);
        compileStmt(catchClause.block.get());
        endScope(); // pops the exception object local variable
        
        int endJump = emitJump(OP_JUMP);
        endJumps.push_back(endJump);

        patchJump(nextCatchJump);
        emit(OP_POP); // Pop the 'matches' boolean for the fallthrough (no match) case
    }
    
    // If no catch block matched, re-throw the exception!
    // Execute the current finally block before throwing
    if (stmt->finallyBlock) {
        compileStmt(stmt->finallyBlock.get());
    }
    emit(OP_THROW);

    patchJump(skipCatchJump);
    
    for (int jump : endJumps) {
        patchJump(jump);
    }
    
    if (stmt->finallyBlock) {
        activeFinallyBlocks.pop_back();
        compileStmt(stmt->finallyBlock.get());
    }
}

}  // namespace vm
