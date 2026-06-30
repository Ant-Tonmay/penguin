#include "parser/parser.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <string>

// Helper macros for verification
#define ASSERT_NOT_NULL(ptr) if (ptr == nullptr) { std::cerr << "Assertion failed: " #ptr " is null at line " << __LINE__ << std::endl; exit(1); }
#define ASSERT_EQ(val1, val2) if (val1 != val2) { std::cerr << "Assertion failed: " #val1 " (" << val1 << ") != " #val2 " (" << val2 << ") at line " << __LINE__ << std::endl; exit(1); }

void testFunctionCallByValue() {
    std::cout << "Testing Function Call By Value..." << std::endl;
    // func f(arr) { }
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "f"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::IDENTIFIER, "arr"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    ASSERT_EQ(program->functions.size(), 1);
    
    auto& func = program->functions[0];
    ASSERT_EQ(func->name, "f");
    ASSERT_EQ(func->params.size(), 1);
    ASSERT_EQ(func->params[0].name, "arr");
    ASSERT_EQ(func->params[0].isRef, false);
}

void testFunctionCallByReference() {
    std::cout << "Testing Function Call By Reference..." << std::endl;
    // func g(ref: brr) { }
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "g"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::IDENTIFIER, "ref"), // This is parsed as identifier "ref"
        Token(TokenType::COLON, ":"),
        Token(TokenType::IDENTIFIER, "brr"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    ASSERT_EQ(program->functions.size(), 1);
    
    auto& func = program->functions[0];
    ASSERT_EQ(func->name, "g");
    ASSERT_EQ(func->params.size(), 1);
    ASSERT_EQ(func->params[0].name, "brr");
    ASSERT_EQ(func->params[0].isRef, true);
}

void testFunctionMixedParams() {
    std::cout << "Testing Function Mixed Params..." << std::endl;
    // func h(arr, ref: brr) { }
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "h"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::IDENTIFIER, "arr"),
        Token(TokenType::COMMA, ","),
        Token(TokenType::IDENTIFIER, "ref"),
        Token(TokenType::COLON, ":"),
        Token(TokenType::IDENTIFIER, "brr"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    ASSERT_EQ(program->functions.size(), 1);
    
    auto& func = program->functions[0];
    ASSERT_EQ(func->name, "h");
    ASSERT_EQ(func->params.size(), 2);
    
    // First param: arr (by value)
    ASSERT_EQ(func->params[0].name, "arr");
    ASSERT_EQ(func->params[0].isRef, false);
    
    // Second param: brr (by reference)
    ASSERT_EQ(func->params[1].name, "brr");
    ASSERT_EQ(func->params[1].isRef, true);
}



void testFunctionWithReturnVoid() {
    std::cout << "Testing Function with Void Return..." << std::endl;
    // func f() { return; }
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "f"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "return"),
        Token(TokenType::SEMICOLON, ";"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    
    ASSERT_EQ(func->body->statements.size(), 1);
    auto* returnStmt = dynamic_cast<ReturnStmt*>(func->body->statements[0].get());
    ASSERT_NOT_NULL(returnStmt);
    if (returnStmt->value != nullptr) {
        std::cerr << "Assertion failed: returnStmt->value is not null" << std::endl;
        exit(1);
    }
}

void testFunctionWithReturnValue() {
    std::cout << "Testing Function with Return Value..." << std::endl;
    // func g() { return 10; }
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "g"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "return"),
        Token(TokenType::NUMBER, "10"),
        Token(TokenType::SEMICOLON, ";"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    
    auto* returnStmt = dynamic_cast<ReturnStmt*>(func->body->statements[0].get());
    ASSERT_NOT_NULL(returnStmt);
    
    auto* numExpr = dynamic_cast<NumberExpr*>(returnStmt->value.get());
    ASSERT_NOT_NULL(numExpr);
    ASSERT_EQ(numExpr->value, "10");
}

void testFunctionWithReturnExpression() {
    std::cout << "Testing Function with Return Expression..." << std::endl;
    // func h(a, b) { return a + b; }
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "h"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::COMMA, ","),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "return"),
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::PLUS, "+"),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::SEMICOLON, ";"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    
    auto* returnStmt = dynamic_cast<ReturnStmt*>(func->body->statements[0].get());
    ASSERT_NOT_NULL(returnStmt);
    
    auto* binExpr = dynamic_cast<BinaryExpr*>(returnStmt->value.get());
    ASSERT_NOT_NULL(binExpr);
    ASSERT_EQ(binExpr->op, "+");
}

int main() {
    try {
        testFunctionCallByValue();
        testFunctionCallByReference();
        testFunctionMixedParams();
        testFunctionWithReturnVoid();
        testFunctionWithReturnValue();
        testFunctionWithReturnExpression();
        std::cout << "All function parser tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
