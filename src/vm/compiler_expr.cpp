#include "vm/compiler.h"

#include "lexer/lexer.h"
#include "parser/parser.h"

namespace vm {

void Compiler::compileExpr(ASTNode* node) {
    if (node) currentLocation = node->loc;
    if (auto* num = dynamic_cast<NumberExpr*>(node)) {
        try {
            const std::string& s = num->value;
            const bool isInt =
                s.find('.') == std::string::npos &&
                s.find('e') == std::string::npos &&
                s.find('E') == std::string::npos;
            if (isInt) {
                emitConstant(static_cast<int64_t>(std::stoll(s)));
            } else {
                emitConstant(std::stod(s));
            }
        } catch (...) {
        }
    } else if (auto* b = dynamic_cast<BoolExpr*>(node)) {
        if (b->value) emit(OP_TRUE);
        else emit(OP_FALSE);
    } else if (auto* c = dynamic_cast<CharExpr*>(node)) {
        emitConstant(c->value);
    } else if (auto* s = dynamic_cast<StringExpr*>(node)) {
        const std::string& str = s->value;
        const bool hasInterpolation = (str.find('{') != std::string::npos);
        if (!hasInterpolation) {
            emitConstant(str);
        } else {
            int partCount = 0;
            size_t i = 0;
            while (i < str.length()) {
                if (str[i] == '{') {
                    size_t j = i + 1;
                    while (j < str.length() && str[j] != '}') j++;
                    if (j < str.length()) {
                        std::string exprStr = str.substr(i + 1, j - i - 1);
                        Lexer lexer(exprStr);
                        auto tokens = lexer.tokenize();
                        Parser parser(tokens);
                        auto expr = parser.parseExpression();
                        compileExpr(expr.get());
                        partCount++;
                        i = j + 1;
                        continue;
                    }
                }

                std::string literal;
                while (i < str.length() && str[i] != '{') {
                    literal += str[i];
                    i++;
                }
                if (!literal.empty()) {
                    emitConstant(literal);
                    partCount++;
                }
            }

            for (int p = 1; p < partCount; p++) {
                emit(OP_ADD);
            }
        }
    } else if (auto* var = dynamic_cast<VarExpr*>(node)) {
        int arg = resolveLocal(var->name);
        if (arg != -1) {
            emit(OP_GET_LOCAL);
            emit(arg);
        } else {
            int thisArg = resolveLocal("this");
            Value nameVal = var->name;
            int idx = currentChunk().addConstant(nameVal);

            if (thisArg != -1) {
                emit(OP_GET_LOCAL);
                emit(thisArg);
                emit(OP_GET_PROPERTY_OR_GLOBAL);
                emit(idx);
            } else {
                emit(OP_GET_GLOBAL);
                emit(idx);
            }
        }
    } else if (auto* call = dynamic_cast<CallExpr*>(node)) {
        if (auto* calleeName = dynamic_cast<VarExpr*>(call->callee.get())) {
            if (calleeName->name == "fixed") {
                for (const auto& arg : call->arguments) {
                    compileExpr(arg.get());
                }
                emit(OP_FIXED_ARRAY);
                emit(static_cast<uint8_t>(call->arguments.size()));
                return;
            }
            if (calleeName->name == "push") {
                for (const auto& arg : call->arguments) {
                    compileExpr(arg.get());
                }
                emit(OP_ARRAY_PUSH);
                return;
            }
            if (calleeName->name == "length") {
                for (const auto& arg : call->arguments) {
                    compileExpr(arg.get());
                }
                emit(OP_ARRAY_LENGTH);
                return;
            }
            if (calleeName->name == "int") {
                for (const auto& arg : call->arguments) {
                    compileExpr(arg.get());
                }
                emit(OP_CAST_INT);
                return;
            }
            if (calleeName->name == "float") {
                for (const auto& arg : call->arguments) {
                    compileExpr(arg.get());
                }
                emit(OP_CAST_FLOAT);
                return;
            }
            if (calleeName->name == "string") {
                for (const auto& arg : call->arguments) {
                    compileExpr(arg.get());
                }
                emit(OP_CAST_STRING);
                return;
            }
            if (calleeName->name == "bool") {
                for (const auto& arg : call->arguments) {
                    compileExpr(arg.get());
                }
                emit(OP_CAST_BOOL);
                return;
            }
            if (calleeName->name == "char") {
                for (const auto& arg : call->arguments) {
                    compileExpr(arg.get());
                }
                emit(OP_CAST_CHAR);
                return;
            }
            if (calleeName->name == "type") {
                for (const auto& arg : call->arguments) {
                    compileExpr(arg.get());
                }
                emit(OP_TYPEOF);
                return;
            }
            if (calleeName->name == "readline") {
                emit(OP_READLINE);
                return;
            }
        }

        if (auto* mem = dynamic_cast<MemberExpr*>(call->callee.get())) {
            if (mem->name == "push") {
                compileExpr(mem->object.get());
                for (const auto& arg : call->arguments) {
                    compileExpr(arg.get());
                }
                emit(OP_ARRAY_PUSH);
                return;
            }

            compileExpr(mem->object.get());
            int nameIdx = currentChunk().addConstant(mem->name);
            emit(OP_GET_PROPERTY);
            emit(nameIdx);

            for (const auto& arg : call->arguments) {
                compileExpr(arg.get());
            }

            emit(OP_CALL);
            emit(static_cast<uint8_t>(call->arguments.size()));
            return;
        }

        compileExpr(call->callee.get());
        for (const auto& arg : call->arguments) {
            compileExpr(arg.get());
        }
        emit(OP_CALL);
        emit(static_cast<uint8_t>(call->arguments.size()));
    } else if (auto* arr = dynamic_cast<ArrayExpr*>(node)) {
        for (const auto& el : arr->elements) {
            compileExpr(el.get());
        }
        emit(OP_NEW_ARRAY);
        emit(static_cast<uint8_t>(arr->elements.size()));
    } else if (auto* idx = dynamic_cast<IndexExpr*>(node)) {
        compileExpr(idx->array.get());
        compileExpr(idx->index.get());
        emit(OP_INDEX_GET);
    } else if (auto* bin = dynamic_cast<BinaryExpr*>(node)) {
        if (bin->op == "&&") {
            compileExpr(bin->left.get());
            int endJump = emitJump(OP_JUMP_IF_FALSE);
            emit(OP_POP);
            compileExpr(bin->right.get());
            patchJump(endJump);
            return;
        }
        if (bin->op == "||") {
            compileExpr(bin->left.get());
            int elseJump = emitJump(OP_JUMP_IF_FALSE);
            int endJump = emitJump(OP_JUMP);

            patchJump(elseJump);
            emit(OP_POP);
            compileExpr(bin->right.get());

            patchJump(endJump);
            return;
        }

        compileExpr(bin->left.get());
        compileExpr(bin->right.get());

        if (bin->op == "+") emit(OP_ADD);
        else if (bin->op == "-") emit(OP_SUB);
        else if (bin->op == "*") emit(OP_MUL);
        else if (bin->op == "/") emit(OP_DIV);
        else if (bin->op == "%") emit(OP_MOD);
        else if (bin->op == ">") emit(OP_GREATER);
        else if (bin->op == ">=") emit(OP_GREATER_EQUAL);
        else if (bin->op == "<") emit(OP_LESSER);
        else if (bin->op == "<=") emit(OP_LESSER_EQUAL);
        else if (bin->op == ">>") emit(OP_RIGHT_SHIFT);
        else if (bin->op == "<<") emit(OP_LEFT_SHIFT);
        else if (bin->op == "|") emit(OP_BITWISE_OR);
        else if (bin->op == "&") emit(OP_BITWISE_AND);
        else if (bin->op == "==") emit(OP_EQUAL);
        else if (bin->op == "!=") emit(OP_NOT_EQUAL);
        else if (bin->op == "^") emit(OP_XOR);
        else if (bin->op == "+=") emit(OP_PLUS_EQUAL);
        else if (bin->op == "-=") emit(OP_MINUS_EQUAL);
        else if (bin->op == "*=") emit(OP_MULTIPLY_EQUAL);
        else if (bin->op == "/=") emit(OP_DIVIDE_EQUAL);
        else if (bin->op == "%=") emit(OP_MODULO_EQUAL);
        else if (bin->op == "<<=") emit(OP_LEFT_SHIFT_EQUAL);
        else if (bin->op == ">>=") emit(OP_RIGHT_SHIFT_EQUAL);
        else if (bin->op == "&=") emit(OP_BITWISE_AND_EQUAL);
        else if (bin->op == "|=") emit(OP_BITWISE_OR_EQUAL);
        else if (bin->op == "^=") emit(OP_XOR_EQUAL);
    } else if (auto* mem = dynamic_cast<MemberExpr*>(node)) {
        compileExpr(mem->object.get());
        int nameIdx = currentChunk().addConstant(mem->name);
        emit(OP_GET_PROPERTY);
        emit(nameIdx);
    }
}

}  // namespace vm
