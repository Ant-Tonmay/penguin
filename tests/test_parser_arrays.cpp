#include "parser/parser.h"
#include "exceptions/error.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <string>

// Helper macros for verification
#define ASSERT_NOT_NULL(ptr) if (ptr == nullptr) { std::cerr << "Assertion failed: " #ptr " is null at line " << __LINE__ << std::endl; exit(1); }
#define ASSERT_EQ(val1, val2) if (val1 != val2) { std::cerr << "Assertion failed: " #val1 " (" << val1 << ") != " #val2 " (" << val2 << ") at line " << __LINE__ << std::endl; exit(1); }

// Helper to wrap tokens in a main function for parsing
std::unique_ptr<Program> parseTokens(std::vector<Token>& tokens) {
    std::vector<Token> progTokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{")
    };
    progTokens.insert(progTokens.end(), tokens.begin(), tokens.end());
    progTokens.push_back(Token(TokenType::RBRACE, "}"));
    progTokens.push_back(Token(TokenType::RBRACE, "}"));
    progTokens.push_back(Token(TokenType::EOF_TOKEN, ""));
    
    Parser parser(progTokens);
    return parser.parse();
}

void testBasicArray() {
    std::cout << "Testing Basic Array: [1, 2, 3]..." << std::endl;
    // arr = [1, 2, 3];
    std::vector<Token> tokens = {
        Token(TokenType::IDENTIFIER, "arr"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::LBRACKET, "["),
        Token(TokenType::NUMBER, "1"),
        Token(TokenType::COMMA, ","),
        Token(TokenType::NUMBER, "2"),
        Token(TokenType::COMMA, ","),
        Token(TokenType::NUMBER, "3"),
        Token(TokenType::RBRACKET, "]"),
        Token(TokenType::SEMICOLON, ";")
    };

    auto program = parseTokens(tokens);
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* assignStmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    ASSERT_NOT_NULL(assignStmt);
    
    auto* arrayExpr = dynamic_cast<ArrayExpr*>(assignStmt->assignments[0].value.get());
    ASSERT_NOT_NULL(arrayExpr);
    ASSERT_EQ(arrayExpr->elements.size(), 3);
}

void testArrayWithCall() {
    std::cout << "Testing Array with Call: [foo(5)]..." << std::endl;
    // brr = [foo(5)];
    std::vector<Token> tokens = {
        Token(TokenType::IDENTIFIER, "brr"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::LBRACKET, "["),
        Token(TokenType::IDENTIFIER, "foo"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::NUMBER, "5"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::RBRACKET, "]"),
        Token(TokenType::SEMICOLON, ";")
    };

    auto program = parseTokens(tokens);
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* assignStmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    
    auto* arrayExpr = dynamic_cast<ArrayExpr*>(assignStmt->assignments[0].value.get());
    ASSERT_NOT_NULL(arrayExpr);
    ASSERT_EQ(arrayExpr->elements.size(), 1);
    
    auto* callExpr = dynamic_cast<CallExpr*>(arrayExpr->elements[0].get());
    ASSERT_NOT_NULL(callExpr);
    
    auto* callee = dynamic_cast<VarExpr*>(callExpr->callee.get());
    ASSERT_NOT_NULL(callee);
    ASSERT_EQ(callee->name, "foo");
    ASSERT_EQ(callExpr->arguments.size(), 1);
}

void testArrayRejectsShapeCall() {
    std::cout << "Testing Array rejects shaped constructor: [shape(5)]..." << std::endl;
    std::vector<Token> tokens = {
        Token(TokenType::IDENTIFIER, "brr"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::LBRACKET, "["),
        Token(TokenType::IDENTIFIER, "shape"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::NUMBER, "5"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::RBRACKET, "]"),
        Token(TokenType::SEMICOLON, ";")
    };

    try {
        auto program = parseTokens(tokens);
        (void)program;
        std::cerr << "Assertion failed: [shape(5)] should be rejected at line " << __LINE__ << std::endl;
        exit(1);
    } catch (const CompileError&) {
    }
}

void testArrayRejectsFixedCall() {
    std::cout << "Testing Array rejects legacy shaped constructor: [fixed(5)]..." << std::endl;
    std::vector<Token> tokens = {
        Token(TokenType::IDENTIFIER, "brr"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::LBRACKET, "["),
        Token(TokenType::IDENTIFIER, "fixed"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::NUMBER, "5"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::RBRACKET, "]"),
        Token(TokenType::SEMICOLON, ";")
    };

    try {
        auto program = parseTokens(tokens);
        (void)program;
        std::cerr << "Assertion failed: [fixed(5)] should be rejected at line " << __LINE__ << std::endl;
        exit(1);
    } catch (const CompileError&) {
    }
}

void testMethodCall() {
    std::cout << "Testing Method Call: crr.push(i)..." << std::endl;
    
    std::vector<Token> tokens = {
        Token(TokenType::IDENTIFIER, "dummy"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::IDENTIFIER, "crr"),
        Token(TokenType::DOT, "."),
        Token(TokenType::IDENTIFIER, "push"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::IDENTIFIER, "i"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::SEMICOLON, ";")
    };

    auto program = parseTokens(tokens);
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* assignStmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    
    auto* methodCall = dynamic_cast<CallExpr*>(assignStmt->assignments[0].value.get());
    ASSERT_NOT_NULL(methodCall);
    
    auto* memberExpr = dynamic_cast<MemberExpr*>(methodCall->callee.get());
    ASSERT_NOT_NULL(memberExpr);
    ASSERT_EQ(memberExpr->name, "push");
    
    auto* obj = dynamic_cast<VarExpr*>(memberExpr->object.get());
    ASSERT_NOT_NULL(obj);
    ASSERT_EQ(obj->name, "crr");
}

void testNestedCalls() {
    std::cout << "Testing Nested Calls: shape(n, shape(n))..." << std::endl;
    // multidim2 = shape(n, shape(n));
    std::vector<Token> tokens = {
        Token(TokenType::IDENTIFIER, "multidim2"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::IDENTIFIER, "shape"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::IDENTIFIER, "n"),
        Token(TokenType::COMMA, ","),
        Token(TokenType::IDENTIFIER, "shape"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::IDENTIFIER, "n"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::SEMICOLON, ";")
    };

    auto program = parseTokens(tokens);
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* assignStmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    
    auto* outerCall = dynamic_cast<CallExpr*>(assignStmt->assignments[0].value.get());
    ASSERT_NOT_NULL(outerCall);
    ASSERT_EQ(outerCall->arguments.size(), 2);
    
    auto* outerCallee = dynamic_cast<VarExpr*>(outerCall->callee.get());
    ASSERT_NOT_NULL(outerCallee);
    ASSERT_EQ(outerCallee->name, "shape");

    auto* innerCall = dynamic_cast<CallExpr*>(outerCall->arguments[1].get());
    ASSERT_NOT_NULL(innerCall);
    ASSERT_EQ(innerCall->arguments.size(), 1);

    auto* innerCallee = dynamic_cast<VarExpr*>(innerCall->callee.get());
    ASSERT_NOT_NULL(innerCallee);
    ASSERT_EQ(innerCallee->name, "shape");
}
void test4DArray() {
    std::cout << "Testing 4D Array..." << std::endl;

    // arr4d = [
    //   [
    //     [ [1], [2] ],
    //     [ [3], [4] ]
    //   ],
    //   [
    //     [ [5], [6] ],
    //     [ [7], [8] ]
    //   ]
    // ];

    std::vector<Token> tokens = {
        Token(TokenType::IDENTIFIER, "arr4d"),
        Token(TokenType::EQUAL, "="),

        Token(TokenType::LBRACKET, "["),

            Token(TokenType::LBRACKET, "["),
                Token(TokenType::LBRACKET, "["),
                    Token(TokenType::LBRACKET, "["),
                        Token(TokenType::NUMBER, "1"),
                    Token(TokenType::RBRACKET, "]"),
                    Token(TokenType::COMMA, ","),
                    Token(TokenType::LBRACKET, "["),
                        Token(TokenType::NUMBER, "2"),
                    Token(TokenType::RBRACKET, "]"),
                Token(TokenType::RBRACKET, "]"),
                Token(TokenType::COMMA, ","),
                Token(TokenType::LBRACKET, "["),
                    Token(TokenType::LBRACKET, "["),
                        Token(TokenType::NUMBER, "3"),
                    Token(TokenType::RBRACKET, "]"),
                    Token(TokenType::COMMA, ","),
                    Token(TokenType::LBRACKET, "["),
                        Token(TokenType::NUMBER, "4"),
                    Token(TokenType::RBRACKET, "]"),
                Token(TokenType::RBRACKET, "]"),
            Token(TokenType::RBRACKET, "]"),

            Token(TokenType::COMMA, ","),

            Token(TokenType::LBRACKET, "["),
                Token(TokenType::LBRACKET, "["),
                    Token(TokenType::LBRACKET, "["),
                        Token(TokenType::NUMBER, "5"),
                    Token(TokenType::RBRACKET, "]"),
                    Token(TokenType::COMMA, ","),
                    Token(TokenType::LBRACKET, "["),
                        Token(TokenType::NUMBER, "6"),
                    Token(TokenType::RBRACKET, "]"),
                Token(TokenType::RBRACKET, "]"),
                Token(TokenType::COMMA, ","),
                Token(TokenType::LBRACKET, "["),
                    Token(TokenType::LBRACKET, "["),
                        Token(TokenType::NUMBER, "7"),
                    Token(TokenType::RBRACKET, "]"),
                    Token(TokenType::COMMA, ","),
                    Token(TokenType::LBRACKET, "["),
                        Token(TokenType::NUMBER, "8"),
                    Token(TokenType::RBRACKET, "]"),
                Token(TokenType::RBRACKET, "]"),
            Token(TokenType::RBRACKET, "]"),

        Token(TokenType::RBRACKET, "]"),
        Token(TokenType::SEMICOLON, ";")
    };

    auto program = parseTokens(tokens);
    ASSERT_NOT_NULL(program);

    auto& func = program->functions[0];
    auto* assignStmt =
        dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    ASSERT_NOT_NULL(assignStmt);

    auto* arr4d =
        dynamic_cast<ArrayExpr*>(assignStmt->assignments[0].value.get());
    ASSERT_NOT_NULL(arr4d);

    // Dimension checks
    ASSERT_EQ(arr4d->elements.size(), 2);                 // D1
    auto* d2 = dynamic_cast<ArrayExpr*>(arr4d->elements[0].get());
    ASSERT_NOT_NULL(d2);
    ASSERT_EQ(d2->elements.size(), 2);                     // D2

    auto* d3 = dynamic_cast<ArrayExpr*>(d2->elements[0].get());
    ASSERT_NOT_NULL(d3);
    ASSERT_EQ(d3->elements.size(), 2);                     // D3

    auto* d4 = dynamic_cast<ArrayExpr*>(d3->elements[0].get());
    ASSERT_NOT_NULL(d4);
    ASSERT_EQ(d4->elements.size(), 1);                     // D4

    auto* value = dynamic_cast<NumberExpr*>(d4->elements[0].get());
    ASSERT_NOT_NULL(value);
}


int main() {
    std::cout << "Running Parser Array Tests..." << std::endl;
    testBasicArray();
    testArrayWithCall();
    testArrayRejectsShapeCall();
    testArrayRejectsFixedCall();
    testMethodCall();
    testNestedCalls();
    test4DArray();
    std::cout << "All array parser tests passed!" << std::endl;
    return 0;
}
