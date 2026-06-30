#include "parser/parser.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <string>

// Helper macros for verification
#define ASSERT_NOT_NULL(ptr) if (ptr == nullptr) { std::cerr << "Assertion failed: " #ptr " is null at line " << __LINE__ << std::endl; exit(1); }
#define ASSERT_EQ(val1, val2) if (val1 != val2) { std::cerr << "Assertion failed: " #val1 " (" << val1 << ") != " #val2 " (" << val2 << ") at line " << __LINE__ << std::endl; exit(1); }
#define ASSERT_ASSIGN_NAME(assignment, expected) \
    { \
        auto* v = dynamic_cast<VarExpr*>((assignment).target.get()); \
        ASSERT_NOT_NULL(v); \
        ASSERT_EQ(v->name, expected); \
    }

void testNestedForLoops() {
    std::cout << "Testing Nested For Loops..." << std::endl;
    // main() { 
    //   for(i=0; i<10; i=i+1) { 
    //     for(j=0; j<5; j=j+1) {
    //       print(i); 
    //     }
    //   } 
    // }
    
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        // Outer loop: for(i=0; i<10; i=i+1)
        Token(TokenType::KEYWORD, "for"), 
        Token(TokenType::LPAREN, "("),
        // Init: i = 0
        Token(TokenType::IDENTIFIER, "i"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::NUMBER, "0"),
        Token(TokenType::SEMICOLON, ";"),
        // Condition: i < 10
        Token(TokenType::IDENTIFIER, "i"),
        Token(TokenType::LESS, "<"),
        Token(TokenType::NUMBER, "10"),
        Token(TokenType::SEMICOLON, ";"),
        // Increment: i = i + 1
        Token(TokenType::IDENTIFIER, "i"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::IDENTIFIER, "i"),
        Token(TokenType::PLUS, "+"),
        Token(TokenType::NUMBER, "1"),
        Token(TokenType::RPAREN, ")"),
        
        // Outer Body Start
        Token(TokenType::LBRACE, "{"),

            // Inner loop: for(j=0; j<5; j=j+1)
            Token(TokenType::KEYWORD, "for"), 
            Token(TokenType::LPAREN, "("),
            // Init: j = 0
            Token(TokenType::IDENTIFIER, "j"),
            Token(TokenType::EQUAL, "="),
            Token(TokenType::NUMBER, "0"),
            Token(TokenType::SEMICOLON, ";"),
            // Condition: j < 5
            Token(TokenType::IDENTIFIER, "j"),
            Token(TokenType::LESS, "<"),
            Token(TokenType::NUMBER, "5"),
            Token(TokenType::SEMICOLON, ";"),
            // Increment: j = j + 1
            Token(TokenType::IDENTIFIER, "j"),
            Token(TokenType::EQUAL, "="),
            Token(TokenType::IDENTIFIER, "j"),
            Token(TokenType::PLUS, "+"),
            Token(TokenType::NUMBER, "1"),
            Token(TokenType::RPAREN, ")"),

            // Inner Body: { print(i); }
            Token(TokenType::LBRACE, "{"),
            Token(TokenType::IDENTIFIER, "print"),
            Token(TokenType::LPAREN, "("),
            Token(TokenType::IDENTIFIER, "i"),
            Token(TokenType::RPAREN, ")"),
            Token(TokenType::SEMICOLON, ";"),
            Token(TokenType::RBRACE, "}"),

        // Outer Body End
        Token(TokenType::RBRACE, "}"),
        
        Token(TokenType::RBRACE, "}"), // end function block
        Token(TokenType::RBRACE, "}"), // end program
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    
    // Outer loop check
    auto* outerFor = dynamic_cast<ForStmt*>(func->body->statements[0].get());
    ASSERT_NOT_NULL(outerFor);
    ASSERT_ASSIGN_NAME(outerFor->init->assignments[0], "i");
    
    // Inner loop check
    auto* innerFor = dynamic_cast<ForStmt*>(outerFor->body->statements[0].get());
    ASSERT_NOT_NULL(innerFor);
    ASSERT_ASSIGN_NAME(innerFor->init->assignments[0], "j");

    // Inner loop body check
    auto* printStmt = dynamic_cast<PrintStmt*>(innerFor->body->statements[0].get());
    ASSERT_NOT_NULL(printStmt);
    // Assuming PrintStmt has an expression that is a VarExpr "i"
    // Adjust based on actual PrintStmt structure if needed, but for now checking existence.
}

void testWhileLoop() {
    std::cout << "Testing While Loop..." << std::endl;
    // main() {
    //   while(i < 5) {
    //     print(i);
    //     i = i + 1;
    //   }
    // }

    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),

        // while(i < 5)
        Token(TokenType::KEYWORD, "while"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::IDENTIFIER, "i"),
        Token(TokenType::LESS, "<"),
        Token(TokenType::NUMBER, "5"),
        Token(TokenType::RPAREN, ")"),

        // { print(i); i = i + 1; }
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::IDENTIFIER, "print"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::IDENTIFIER, "i"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::SEMICOLON, ";"),

        Token(TokenType::IDENTIFIER, "i"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::IDENTIFIER, "i"),
        Token(TokenType::PLUS, "+"),
        Token(TokenType::NUMBER, "1"),
        Token(TokenType::SEMICOLON, ";"),
        Token(TokenType::RBRACE, "}"),

        Token(TokenType::RBRACE, "}"), // end function block
        Token(TokenType::RBRACE, "}"), // end program
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];

    // Check WhileStmt
    auto* whileStmt = dynamic_cast<WhileStmt*>(func->body->statements[0].get());
    ASSERT_NOT_NULL(whileStmt);

    // Check Condition: i < 5
    auto* condition = dynamic_cast<BinaryExpr*>(whileStmt->condition.get());
    ASSERT_NOT_NULL(condition);
    ASSERT_EQ(condition->op, "<");

    // Check Body
    ASSERT_EQ(whileStmt->body->statements.size(), 2);
}

int main() {
    try {
        testNestedForLoops();
        testWhileLoop();
        std::cout << "All nested loop tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
