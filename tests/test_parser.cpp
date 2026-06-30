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

void testNumber() {
    std::cout << "Testing Number..." << std::endl;
    std::vector<Token> tokens = {
        Token(TokenType::NUMBER, "123"),
        Token(TokenType::EOF_TOKEN, "")
    };
    Parser parser(tokens);
    auto expr = parser.parseExpression(); // Changed to parseExpression

    auto* num = dynamic_cast<NumberExpr*>(expr.get());
    ASSERT_NOT_NULL(num);
    ASSERT_EQ(num->value, "123");
}

void testBinaryOp() {
    std::cout << "Testing BinaryOp..." << std::endl;
    std::vector<Token> tokens = {
        Token(TokenType::NUMBER, "1"),
        Token(TokenType::PLUS, "+"),
        Token(TokenType::NUMBER, "2"),
        Token(TokenType::EOF_TOKEN, "")
    };
    Parser parser(tokens);
    auto expr = parser.parseExpression();

    auto* bin = dynamic_cast<BinaryExpr*>(expr.get());
    ASSERT_NOT_NULL(bin);
    ASSERT_EQ(bin->op, "+");
    
    auto* left = dynamic_cast<NumberExpr*>(bin->left.get());
    ASSERT_NOT_NULL(left);
    ASSERT_EQ(left->value, "1");

    auto* right = dynamic_cast<NumberExpr*>(bin->right.get());
    ASSERT_NOT_NULL(right);
    ASSERT_EQ(right->value, "2");
}

void testPrecedence() {
    std::cout << "Testing Precedence..." << std::endl;
    // "1 + 2 * 3" -> "1 + (2 * 3)"
    std::vector<Token> tokens = {
        Token(TokenType::NUMBER, "1"),
        Token(TokenType::PLUS, "+"),
        Token(TokenType::NUMBER, "2"),
        Token(TokenType::STAR, "*"),
        Token(TokenType::NUMBER, "3"),
        Token(TokenType::EOF_TOKEN, "")
    };
    Parser parser(tokens);
    auto expr = parser.parseExpression();

    auto* root = dynamic_cast<BinaryExpr*>(expr.get());
    ASSERT_NOT_NULL(root);
    ASSERT_EQ(root->op, "+");

    auto* left = dynamic_cast<NumberExpr*>(root->left.get());
    ASSERT_NOT_NULL(left);
    ASSERT_EQ(left->value, "1");

    auto* rightBin = dynamic_cast<BinaryExpr*>(root->right.get());
    ASSERT_NOT_NULL(rightBin);
    ASSERT_EQ(rightBin->op, "*");
}

void testGrouping() {
    std::cout << "Testing Grouping..." << std::endl;
    // "(1 + 2) * 3"
    std::vector<Token> tokens = {
        Token(TokenType::LPAREN, "("),
        Token(TokenType::NUMBER, "1"),
        Token(TokenType::PLUS, "+"),
        Token(TokenType::NUMBER, "2"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::STAR, "*"),
        Token(TokenType::NUMBER, "3"),
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto expr = parser.parseExpression();

    auto* root = dynamic_cast<BinaryExpr*>(expr.get());
    ASSERT_NOT_NULL(root);
    ASSERT_EQ(root->op, "*");

    auto* leftBin = dynamic_cast<BinaryExpr*>(root->left.get());
    ASSERT_NOT_NULL(leftBin);
    ASSERT_EQ(leftBin->op, "+");
}

void testAssignmentStmt() {
    std::cout << "Testing AssignmentStmt..." << std::endl;
    // a = 10, b = 20;
    std::vector<Token> tokens = {
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::NUMBER, "10"),
        Token(TokenType::COMMA, ","),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::NUMBER, "20"),
        Token(TokenType::SEMICOLON, ";"),
        Token(TokenType::EOF_TOKEN, "")
    };
    
    // We need to wrap this in a block or handle it via parseBlock since parseStatement is private?
    // Actually parse() -> Program -> Function -> Block -> Statement
    // But we can check if we can expose parseStatement or just wrap it in a mock?
    // Or just test Program structure.
    
    // Let's create a full program structure for statement testing to match public API
    // { main() { a=10, b=20; } }
    
    std::vector<Token> progTokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::NUMBER, "10"),
        Token(TokenType::COMMA, ","),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::NUMBER, "20"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::EOF_TOKEN, "")
    };
    
    Parser parser(progTokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    ASSERT_EQ(program->functions.size(), 1);
    
    auto& func = program->functions[0];
    ASSERT_EQ(func->name, "main");
    ASSERT_EQ(func->body->statements.size(), 1);
    
    auto* stmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->assignments.size(), 2);
    ASSERT_ASSIGN_NAME(stmt->assignments[0], "a");
    ASSERT_ASSIGN_NAME(stmt->assignments[1], "b");
}

void testForStmt() {
    std::cout << "Testing ForStmt..." << std::endl;
    // main() { for(i=0; i<10; i=i+1) { print(i); } }
    
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
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
        
        // Body: { print(i); }
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::IDENTIFIER, "print"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::IDENTIFIER, "i"),
        Token(TokenType::RPAREN, ")"),
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
    auto* forStmt = dynamic_cast<ForStmt*>(func->body->statements[0].get());
    
    ASSERT_NOT_NULL(forStmt);
    ASSERT_NOT_NULL(forStmt->init);
    ASSERT_NOT_NULL(forStmt->condition);
    ASSERT_NOT_NULL(forStmt->increment);
    ASSERT_NOT_NULL(forStmt->body);
    
    // Check init: i=0
    ASSERT_ASSIGN_NAME(forStmt->init->assignments[0], "i");
    
    // Check increment: i=i+1
    ASSERT_ASSIGN_NAME(forStmt->increment->assignments[0], "i");
}

void test_logical_operators(){
    std::cout << "a && b || c" << std::endl;
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::IDENTIFIER, "res"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::AND, "&&"),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::OR, "||"),
        Token(TokenType::IDENTIFIER, "c"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"), // end function block
        Token(TokenType::RBRACE, "}"), // end program
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* assignStmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    
    ASSERT_NOT_NULL(assignStmt);
    ASSERT_ASSIGN_NAME(assignStmt->assignments[0], "res");
    
    // Check expression: a && b || c
    auto* expr = assignStmt->assignments[0].value.get();
    auto* binExpr = dynamic_cast<BinaryExpr*>(expr);
    ASSERT_NOT_NULL(binExpr);
    ASSERT_EQ(binExpr->op, "||");
    
    // Check right side: c
    auto* rightId = dynamic_cast<VarExpr*>(binExpr->right.get());
    ASSERT_NOT_NULL(rightId);
    ASSERT_EQ(rightId->name, "c");
    
    // Check left side: a && b
    auto* leftBin = dynamic_cast<BinaryExpr*>(binExpr->left.get());
    ASSERT_NOT_NULL(leftBin);
    ASSERT_EQ(leftBin->op, "&&");
    
    // Check left-left: a
    auto* leftLeftId = dynamic_cast<VarExpr*>(leftBin->left.get());
    ASSERT_NOT_NULL(leftLeftId);
    ASSERT_EQ(leftLeftId->name, "a");
    
    // Check left-right: b
    auto* leftRightId = dynamic_cast<VarExpr*>(leftBin->right.get());
    ASSERT_NOT_NULL(leftRightId);
    ASSERT_EQ(leftRightId->name, "b");
}

void test_logical_operator_level2(){
    std::cout << "a < b && c < d" << std::endl;
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::IDENTIFIER, "res"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::LESS, "<"),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::AND, "&&"),
        Token(TokenType::IDENTIFIER, "c"),
        Token(TokenType::LESS, "<"),
        Token(TokenType::IDENTIFIER, "d"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"), // end function block
        Token(TokenType::RBRACE, "}"), // end program
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* assignStmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    
    ASSERT_NOT_NULL(assignStmt);
    ASSERT_ASSIGN_NAME(assignStmt->assignments[0], "res");
    
    // Check expression: a < b && c < d
    auto* expr = assignStmt->assignments[0].value.get();
    auto* binExpr = dynamic_cast<BinaryExpr*>(expr);
    ASSERT_NOT_NULL(binExpr);
    ASSERT_EQ(binExpr->op, "&&");
    
    // Check right side: c < d
    auto* rightBin = dynamic_cast<BinaryExpr*>(binExpr->right.get());
    ASSERT_NOT_NULL(rightBin);
    ASSERT_EQ(rightBin->op, "<");
    
    // Check left side: a < b
    auto* leftBin = dynamic_cast<BinaryExpr*>(binExpr->left.get());
    ASSERT_NOT_NULL(leftBin);
    ASSERT_EQ(leftBin->op, "<");
    
    // Check left-left: a
    auto* leftLeftId = dynamic_cast<VarExpr*>(leftBin->left.get());
    ASSERT_NOT_NULL(leftLeftId);
    ASSERT_EQ(leftLeftId->name, "a");
    
    // Check left-right: b
    auto* leftRightId = dynamic_cast<VarExpr*>(leftBin->right.get());
    ASSERT_NOT_NULL(leftRightId);
    ASSERT_EQ(leftRightId->name, "b");
}

void test_logical_operator_level3(){
    std::cout << "a <= b && c >= d || e != f" << std::endl;
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::IDENTIFIER, "res"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::LESS_EQUAL, "<="),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::AND, "&&"),
        Token(TokenType::IDENTIFIER, "c"),
        Token(TokenType::GREATER_EQUAL, ">="),
        Token(TokenType::IDENTIFIER, "d"),
        Token(TokenType::OR, "||"),
        Token(TokenType::IDENTIFIER, "e"),
        Token(TokenType::NOT_EQUAL, "!="),
        Token(TokenType::IDENTIFIER, "f"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"), // end function block
        Token(TokenType::RBRACE, "}"), // end program
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* assignStmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    
    ASSERT_NOT_NULL(assignStmt);
    ASSERT_ASSIGN_NAME(assignStmt->assignments[0], "res");
    
    // Check expression: a <= b && c >= d || e != f
    auto* expr = assignStmt->assignments[0].value.get();
    auto* binExpr = dynamic_cast<BinaryExpr*>(expr);
    ASSERT_NOT_NULL(binExpr);
    ASSERT_EQ(binExpr->op, "||");
    
    // Check right side: e != f
    auto* rightBin = dynamic_cast<BinaryExpr*>(binExpr->right.get());
    ASSERT_NOT_NULL(rightBin);
    ASSERT_EQ(rightBin->op, "!=");
    
    // Check left side: a <= b && c >= d
    auto* leftBin = dynamic_cast<BinaryExpr*>(binExpr->left.get());
    ASSERT_NOT_NULL(leftBin);
    ASSERT_EQ(leftBin->op, "&&");
    
    // Check left-left: a <= b
    auto* leftLeftBin = dynamic_cast<BinaryExpr*>(leftBin->left.get());
    ASSERT_NOT_NULL(leftLeftBin);
    ASSERT_EQ(leftLeftBin->op, "<=");
    
    // Check left-right: c >= d
    auto* leftRightBin = dynamic_cast<BinaryExpr*>(leftBin->right.get());
    ASSERT_NOT_NULL(leftRightBin);
    ASSERT_EQ(leftRightBin->op, ">=");
    
    // Check left-left-left: a
    auto* leftLeftLeftId = dynamic_cast<VarExpr*>(leftLeftBin->left.get());
    ASSERT_NOT_NULL(leftLeftLeftId);
    ASSERT_EQ(leftLeftLeftId->name, "a");
    
    // Check left-left-right: b
    auto* leftLeftRightId = dynamic_cast<VarExpr*>(leftLeftBin->right.get());
    ASSERT_NOT_NULL(leftLeftRightId);
    ASSERT_EQ(leftLeftRightId->name, "b");
    
    // Check left-right-left: c
    auto* leftRightLeftId = dynamic_cast<VarExpr*>(leftRightBin->left.get());
    ASSERT_NOT_NULL(leftRightLeftId);
    ASSERT_EQ(leftRightLeftId->name, "c");
    
    // Check left-right-right: d
    auto* leftRightRightId = dynamic_cast<VarExpr*>(leftRightBin->right.get());
    ASSERT_NOT_NULL(leftRightRightId);
    ASSERT_EQ(leftRightRightId->name, "d");
    
    // Check right-left: e
    auto* rightLeftId = dynamic_cast<VarExpr*>(rightBin->left.get());
    ASSERT_NOT_NULL(rightLeftId);
    ASSERT_EQ(rightLeftId->name, "e");
    
    // Check right-right: f
    auto* rightRightId = dynamic_cast<VarExpr*>(rightBin->right.get());
    ASSERT_NOT_NULL(rightRightId);
    ASSERT_EQ(rightRightId->name, "f");
}

void test_logical_operator_level4(){
    std::cout << "a <= b && c >= d || e != f && g > h" << std::endl;
    //a <= b && c >= d || e != f && g > h
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::IDENTIFIER, "res"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::LESS_EQUAL, "<="),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::AND, "&&"),
        Token(TokenType::IDENTIFIER, "c"),
        Token(TokenType::GREATER_EQUAL, ">="),
        Token(TokenType::IDENTIFIER, "d"),
        Token(TokenType::OR, "||"),
        Token(TokenType::IDENTIFIER, "e"),
        Token(TokenType::NOT_EQUAL, "!="),
        Token(TokenType::IDENTIFIER, "f"),
        Token(TokenType::AND, "&&"),
        Token(TokenType::IDENTIFIER, "g"),
        Token(TokenType::GREATER, ">"),
        Token(TokenType::IDENTIFIER, "h"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"), // end function block
        Token(TokenType::RBRACE, "}"), // end program
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* assignStmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    
    ASSERT_NOT_NULL(assignStmt);
    ASSERT_ASSIGN_NAME(assignStmt->assignments[0], "res");
    
    // Check expression: a <= b && c >= d || e != f && g > h
    auto* expr = assignStmt->assignments[0].value.get();
    auto* binExpr = dynamic_cast<BinaryExpr*>(expr);
    ASSERT_NOT_NULL(binExpr);
    ASSERT_EQ(binExpr->op, "||");
    
    // Check left side: a <= b && c >= d
    auto* leftBin = dynamic_cast<BinaryExpr*>(binExpr->left.get());
    ASSERT_NOT_NULL(leftBin);
    ASSERT_EQ(leftBin->op, "&&");
    
    // Check left-left: a <= b
    auto* leftLeftBin = dynamic_cast<BinaryExpr*>(leftBin->left.get());
    ASSERT_NOT_NULL(leftLeftBin);
    ASSERT_EQ(leftLeftBin->op, "<=");
    
    // Check left-right: c >= d
    auto* leftRightBin = dynamic_cast<BinaryExpr*>(leftBin->right.get());
    ASSERT_NOT_NULL(leftRightBin);
    ASSERT_EQ(leftRightBin->op, ">=");
    
    // Check left-left-left: a
    auto* leftLeftLeftId = dynamic_cast<VarExpr*>(leftLeftBin->left.get());
    ASSERT_NOT_NULL(leftLeftLeftId);
    ASSERT_EQ(leftLeftLeftId->name, "a");
    
    // Check left-left-right: b
    auto* leftLeftRightId = dynamic_cast<VarExpr*>(leftLeftBin->right.get());
    ASSERT_NOT_NULL(leftLeftRightId);
    ASSERT_EQ(leftLeftRightId->name, "b");
    
    // Check left-right-left: c
    auto* leftRightLeftId = dynamic_cast<VarExpr*>(leftRightBin->left.get());
    ASSERT_NOT_NULL(leftRightLeftId);
    ASSERT_EQ(leftRightLeftId->name, "c");
    
    // Check left-right-right: d
    auto* leftRightRightId = dynamic_cast<VarExpr*>(leftRightBin->right.get());
    ASSERT_NOT_NULL(leftRightRightId);
    ASSERT_EQ(leftRightRightId->name, "d");

    // Check right side: e != f && g > h
    // rightBin is "&&"
    auto* rightBin = dynamic_cast<BinaryExpr*>(binExpr->right.get());
    ASSERT_NOT_NULL(rightBin);
    ASSERT_EQ(rightBin->op, "&&");
    
    // Check left side of rightBin: e != f
    auto* rightLeftBin = dynamic_cast<BinaryExpr*>(rightBin->left.get());
    ASSERT_NOT_NULL(rightLeftBin);
    ASSERT_EQ(rightLeftBin->op, "!=");

    // Check rightLeftBin left: e
    auto* eId = dynamic_cast<VarExpr*>(rightLeftBin->left.get());
    ASSERT_NOT_NULL(eId);
    ASSERT_EQ(eId->name, "e");

    // Check rightLeftBin right: f
    auto* fId = dynamic_cast<VarExpr*>(rightLeftBin->right.get());
    ASSERT_NOT_NULL(fId);
    ASSERT_EQ(fId->name, "f");
    
    // Check right side of rightBin: g > h
    auto* rightRightBin = dynamic_cast<BinaryExpr*>(rightBin->right.get());
    ASSERT_NOT_NULL(rightRightBin);
    ASSERT_EQ(rightRightBin->op, ">");

    // Check rightRightBin left: g
    auto* gId = dynamic_cast<VarExpr*>(rightRightBin->left.get());
    ASSERT_NOT_NULL(gId);
    ASSERT_EQ(gId->name, "g");

    // Check rightRightBin right: h
    auto* hId = dynamic_cast<VarExpr*>(rightRightBin->right.get());
    ASSERT_NOT_NULL(hId);
    ASSERT_EQ(hId->name, "h");
}
void test_logical_operator_level5(){
    std::cout << "(a <= b && c >= d || e != f && g > h)" << std::endl;
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::IDENTIFIER, "res"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::LESS_EQUAL, "<="),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::AND, "&&"),
        Token(TokenType::IDENTIFIER, "c"),
        Token(TokenType::GREATER_EQUAL, ">="),
        Token(TokenType::IDENTIFIER, "d"),
        Token(TokenType::OR, "||"),
        Token(TokenType::IDENTIFIER, "e"),
        Token(TokenType::NOT_EQUAL, "!="),
        Token(TokenType::IDENTIFIER, "f"),
        Token(TokenType::AND, "&&"),
        Token(TokenType::IDENTIFIER, "g"),
        Token(TokenType::GREATER, ">"),
        Token(TokenType::IDENTIFIER, "h"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"), // end function block
        Token(TokenType::RBRACE, "}"), // end program
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* assignStmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    
    ASSERT_NOT_NULL(assignStmt);
    ASSERT_ASSIGN_NAME(assignStmt->assignments[0], "res");
    
    // Check expression: a <= b && c >= d || e != f && g > h
    auto* expr = assignStmt->assignments[0].value.get();
    auto* binExpr = dynamic_cast<BinaryExpr*>(expr);
    ASSERT_NOT_NULL(binExpr);
    ASSERT_EQ(binExpr->op, "||");
    
    // Check right side: e != f && g > h
    auto* rightBin = dynamic_cast<BinaryExpr*>(binExpr->right.get());
    ASSERT_NOT_NULL(rightBin);
    ASSERT_EQ(rightBin->op, "&&");
    
    // Check left side: a <= b && c >= d
    auto* leftBin = dynamic_cast<BinaryExpr*>(binExpr->left.get());
    ASSERT_NOT_NULL(leftBin);
    ASSERT_EQ(leftBin->op, "&&");
    
    // Check left-left: a <= b
    auto* leftLeftBin = dynamic_cast<BinaryExpr*>(leftBin->left.get());
    ASSERT_NOT_NULL(leftLeftBin);
    ASSERT_EQ(leftLeftBin->op, "<=");
    
    // Check left-right: c >= d
    auto* leftRightBin = dynamic_cast<BinaryExpr*>(leftBin->right.get());
    ASSERT_NOT_NULL(leftRightBin);
    ASSERT_EQ(leftRightBin->op, ">=");
    
    // Check left-left-left: a
    auto* leftLeftLeftId = dynamic_cast<VarExpr*>(leftLeftBin->left.get());
    ASSERT_NOT_NULL(leftLeftLeftId);
    ASSERT_EQ(leftLeftLeftId->name, "a");
    
    // Check left-left-right: b
    auto* leftLeftRightId = dynamic_cast<VarExpr*>(leftLeftBin->right.get());
    ASSERT_NOT_NULL(leftLeftRightId);
    ASSERT_EQ(leftLeftRightId->name, "b");
    
    // Check left-right-left: c
    auto* leftRightLeftId = dynamic_cast<VarExpr*>(leftRightBin->left.get());
    ASSERT_NOT_NULL(leftRightLeftId);
    ASSERT_EQ(leftRightLeftId->name, "c");
    
    // Check left-right-right: d
    auto* leftRightRightId = dynamic_cast<VarExpr*>(leftRightBin->right.get());
    ASSERT_NOT_NULL(leftRightRightId);
    ASSERT_EQ(leftRightRightId->name, "d");
    
    // Check right side of rightBin: e != f
    auto* rightLeftBin = dynamic_cast<BinaryExpr*>(rightBin->left.get());
    ASSERT_NOT_NULL(rightLeftBin);
    ASSERT_EQ(rightLeftBin->op, "!=");

    // Check rightLeftBin left: e
    auto* eId = dynamic_cast<VarExpr*>(rightLeftBin->left.get());
    ASSERT_NOT_NULL(eId);
    ASSERT_EQ(eId->name, "e");

    // Check rightLeftBin right: f
    auto* fId = dynamic_cast<VarExpr*>(rightLeftBin->right.get());
    ASSERT_NOT_NULL(fId);
    ASSERT_EQ(fId->name, "f");
    
    // Check right side of rightBin: g > h
    auto* rightRightBin = dynamic_cast<BinaryExpr*>(rightBin->right.get());
    ASSERT_NOT_NULL(rightRightBin);
    ASSERT_EQ(rightRightBin->op, ">");

    // Check rightRightBin left: g
    auto* gId = dynamic_cast<VarExpr*>(rightRightBin->left.get());
    ASSERT_NOT_NULL(gId);
    ASSERT_EQ(gId->name, "g");

    // Check rightRightBin right: h
    auto* hId = dynamic_cast<VarExpr*>(rightRightBin->right.get());
    ASSERT_NOT_NULL(hId);
    ASSERT_EQ(hId->name, "h");    
}

void test_left_shift(){
    std::cout << "(a << b)" << std::endl;
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::IDENTIFIER, "res"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::LEFT_SHIFT, "<<"),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"), // end function block
        Token(TokenType::RBRACE, "}"), // end program
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* assignStmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    
    ASSERT_NOT_NULL(assignStmt);
    ASSERT_ASSIGN_NAME(assignStmt->assignments[0], "res");
    
    // Check expression: a << b
    auto* expr = assignStmt->assignments[0].value.get();
    auto* binExpr = dynamic_cast<BinaryExpr*>(expr);
    ASSERT_NOT_NULL(binExpr);
    ASSERT_EQ(binExpr->op, "<<");
    
    // Check left side: a
    auto* leftId = dynamic_cast<VarExpr*>(binExpr->left.get());
    ASSERT_NOT_NULL(leftId);
    ASSERT_EQ(leftId->name, "a");
    
    // Check right side: b
    auto* rightId = dynamic_cast<VarExpr*>(binExpr->right.get());
    ASSERT_NOT_NULL(rightId);
    ASSERT_EQ(rightId->name, "b");
}

void test_right_shift(){
    std::cout << "(a >> b)" << std::endl;
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::IDENTIFIER, "res"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::RIGHT_SHIFT, ">>"),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"), // end function block
        Token(TokenType::RBRACE, "}"), // end program
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* assignStmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    
    ASSERT_NOT_NULL(assignStmt);
    ASSERT_ASSIGN_NAME(assignStmt->assignments[0], "res");
    
    // Check expression: a >> b
    auto* expr = assignStmt->assignments[0].value.get();
    auto* binExpr = dynamic_cast<BinaryExpr*>(expr);
    ASSERT_NOT_NULL(binExpr);
    ASSERT_EQ(binExpr->op, ">>");
    
    // Check left side: a
    auto* leftId = dynamic_cast<VarExpr*>(binExpr->left.get());
    ASSERT_NOT_NULL(leftId);
    ASSERT_EQ(leftId->name, "a");
    
    // Check right side: b
    auto* rightId = dynamic_cast<VarExpr*>(binExpr->right.get());
    ASSERT_NOT_NULL(rightId);
    ASSERT_EQ(rightId->name, "b");
}

void test_shift_operators(){
    std::cout << "(a << b >> c)" << std::endl;
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::IDENTIFIER, "res"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::LEFT_SHIFT, "<<"),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::RIGHT_SHIFT, ">>"),
        Token(TokenType::IDENTIFIER, "c"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"), // end function block
        Token(TokenType::RBRACE, "}"), // end program
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* assignStmt = dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    
    ASSERT_NOT_NULL(assignStmt);
    ASSERT_ASSIGN_NAME(assignStmt->assignments[0], "res");
    
    // Check expression: a << b >> c
    auto* expr = assignStmt->assignments[0].value.get();
    auto* binExpr = dynamic_cast<BinaryExpr*>(expr);
    ASSERT_NOT_NULL(binExpr);
    ASSERT_EQ(binExpr->op, ">>"  );
    
    // Check left side: a << b
    auto* leftBin = dynamic_cast<BinaryExpr*>(binExpr->left.get());
    ASSERT_NOT_NULL(leftBin);
    ASSERT_EQ(leftBin->op, "<<");
    
    // Check left-left: a
    auto* leftLeftId = dynamic_cast<VarExpr*>(leftBin->left.get());
    ASSERT_NOT_NULL(leftLeftId);
    ASSERT_EQ(leftLeftId->name, "a");
    
    // Check left-right: b
    auto* leftRightId = dynamic_cast<VarExpr*>(leftBin->right.get());
    ASSERT_NOT_NULL(leftRightId);
    ASSERT_EQ(leftRightId->name, "b");
    
    // Check right side: c
    auto* rightId = dynamic_cast<VarExpr*>(binExpr->right.get());
    ASSERT_NOT_NULL(rightId);
    ASSERT_EQ(rightId->name, "c");
}

void test_shift_with_parentheses(){
    std::cout << "(a << (b >> c))" << std::endl;

    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::IDENTIFIER, "res"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::IDENTIFIER, "a"),
        Token(TokenType::LEFT_SHIFT, "<<"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::IDENTIFIER, "b"),
        Token(TokenType::RIGHT_SHIFT, ">>"),
        Token(TokenType::IDENTIFIER, "c"),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);

    auto& func = program->functions[0];
    auto* assignStmt =
        dynamic_cast<AssignmentStmt*>(func->body->statements[0].get());
    ASSERT_NOT_NULL(assignStmt);

    ASSERT_ASSIGN_NAME(assignStmt->assignments[0], "res");

    // res = a << (b >> c)
    auto* expr = assignStmt->assignments[0].value.get();
    auto* topBin = dynamic_cast<BinaryExpr*>(expr);
    ASSERT_NOT_NULL(topBin);

    ASSERT_EQ(topBin->op, "<<");

    // Left: a
    auto* leftId = dynamic_cast<VarExpr*>(topBin->left.get());
    ASSERT_NOT_NULL(leftId);
    ASSERT_EQ(leftId->name, "a");

    // Right: (b >> c)
    auto* rightBin = dynamic_cast<BinaryExpr*>(topBin->right.get());
    ASSERT_NOT_NULL(rightBin);
    ASSERT_EQ(rightBin->op, ">>");

    auto* rightLeft = dynamic_cast<VarExpr*>(rightBin->left.get());
    auto* rightRight = dynamic_cast<VarExpr*>(rightBin->right.get());
    ASSERT_NOT_NULL(rightLeft);
    ASSERT_NOT_NULL(rightRight);
    ASSERT_EQ(rightLeft->name, "b");
    ASSERT_EQ(rightRight->name, "c");
}

void test_include_stmt() {
    std::cout << "Testing IncludeStmt..." << std::endl;
    // include math;
    std::vector<Token> tokens1 = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::KEYWORD, "include"),
        Token(TokenType::IDENTIFIER, "math"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser1(tokens1);
    auto program1 = parser1.parse();
    ASSERT_NOT_NULL(program1);
    auto& func1 = program1->functions[0];
    auto* incStmt = dynamic_cast<IncludeStmt*>(func1->body->statements[0].get());
    
    ASSERT_NOT_NULL(incStmt);
    ASSERT_EQ(incStmt->name, "math");
    ASSERT_EQ(incStmt->members.size(), 0);

    // include <sin, cos> from math;
    std::vector<Token> tokens2 = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::KEYWORD, "include"),
        Token(TokenType::LESS, "<"),
        Token(TokenType::IDENTIFIER, "sin"),
        Token(TokenType::COMMA, ","),
        Token(TokenType::IDENTIFIER, "cos"),
        Token(TokenType::GREATER, ">"),
        Token(TokenType::KEYWORD, "from"),
        Token(TokenType::IDENTIFIER, "math"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser2(tokens2);
    auto program2 = parser2.parse();
    ASSERT_NOT_NULL(program2);
    auto& func2 = program2->functions[0];
    auto* incStmt2 = dynamic_cast<IncludeStmt*>(func2->body->statements[0].get());
    
    ASSERT_NOT_NULL(incStmt2);
    ASSERT_EQ(incStmt2->name, "math");
    ASSERT_EQ(incStmt2->members.size(), 2);
    ASSERT_EQ(incStmt2->members[0], "sin");
    ASSERT_EQ(incStmt2->members[1], "cos");
}

void test_alias_stmt() {
    std::cout << "Testing AliasStmt..." << std::endl;
    // alias m = include math;
    std::vector<Token> tokens = {
        Token(TokenType::LBRACE, "{"),
        Token(TokenType::KEYWORD, "func"),
        Token(TokenType::IDENTIFIER, "main"),
        Token(TokenType::LPAREN, "("),
        Token(TokenType::RPAREN, ")"),
        Token(TokenType::LBRACE, "{"),
        
        Token(TokenType::KEYWORD, "alias"),
        Token(TokenType::IDENTIFIER, "m"),
        Token(TokenType::EQUAL, "="),
        Token(TokenType::KEYWORD, "include"),
        Token(TokenType::IDENTIFIER, "math"),
        Token(TokenType::SEMICOLON, ";"),
        
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::RBRACE, "}"),
        Token(TokenType::EOF_TOKEN, "")
    };

    Parser parser(tokens);
    auto program = parser.parse();
    ASSERT_NOT_NULL(program);
    auto& func = program->functions[0];
    auto* aliasStmt = dynamic_cast<AliasStmt*>(func->body->statements[0].get());
    
    ASSERT_NOT_NULL(aliasStmt);
    ASSERT_EQ(aliasStmt->name, "m");
    ASSERT_NOT_NULL(aliasStmt->include);
    ASSERT_EQ(aliasStmt->include->name, "math");
    ASSERT_EQ(aliasStmt->include->alias, "m");
}

void test_array_defination(){
    
}


int main() {
    std::cout << "Running Parser Tests..." << std::endl;
    testNumber();
    testBinaryOp();
    testPrecedence();
    testGrouping();
    testAssignmentStmt();
    testForStmt();
    test_logical_operators();
    test_logical_operator_level2();
    test_logical_operator_level3();
    test_logical_operator_level4();
    test_logical_operator_level5();
    test_shift_operators();
    test_left_shift();
    test_right_shift();
    test_shift_with_parentheses();
    test_include_stmt();
    test_alias_stmt();
    std::cout << "All parser tests passed!" << std::endl;
    return 0;
}
