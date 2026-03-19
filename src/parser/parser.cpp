#include "parser/parser.h"
#include <iostream>
#include <stdexcept>


Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    
    consume(TokenType::LBRACE, "Expect '{' at start of program.");
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        if (check(TokenType::KEYWORD) && peek().lexeme == "class") {
            program->classes.push_back(parseClassStmt());
        } else if (check(TokenType::KEYWORD) && peek().lexeme == "trait") {
            program->traits.push_back(parseTraitStmt());
        } else {
            program->functions.push_back(parseFunction());
        }
    }
    
    consume(TokenType::RBRACE, "Expect '}' at end of program.");
    return program;
}

bool Parser::isAssignmentOperator(TokenType t) {
    switch (t) {
        case TokenType::EQUAL:
        case TokenType::PLUS_EQUAL:
        case TokenType::MINUS_EQUAL:
        case TokenType::STAR_EQUAL:
        case TokenType::SLASH_EQUAL:
        case TokenType::MOD_OP_EQUAL:
        case TokenType::BITWISE_AND_EQUAL:
        case TokenType::BITWISE_OR_EQUAL:
        case TokenType::XOR_EQUAL:
            return true;
        default:
            return false;
    }
}

std::unique_ptr<Block> Parser::parseBlock() {
    consume(TokenType::LBRACE, "Expect '{' to start block.");
    auto block = std::make_unique<Block>();
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        block->statements.push_back(parseStatement());
    }
    
    consume(TokenType::RBRACE, "Expect '}' to end block.");
    return block;
}

std::unique_ptr<BreakStmt> Parser::parseBreakStmt(){
    consume(TokenType::SEMICOLON, "Expect ';' after break");
    return std::make_unique<BreakStmt>();
}
std::unique_ptr<ContinueStmt> Parser::parseContinueStmt(){
    consume(TokenType::SEMICOLON, "Expect ';' after continue");
    return std::make_unique<ContinueStmt>();
}

std::unique_ptr<Stmt> Parser::parseStatement() {

    if (check(TokenType::KEYWORD) && peek().lexeme == "break") {
        advance();
        return parseBreakStmt();
    }

    if (check(TokenType::KEYWORD) && peek().lexeme == "continue") {
        advance();
        return parseContinueStmt();
    }

  
    if (check(TokenType::IDENTIFIER) && peek().lexeme == "print") {
        auto stmt = parsePrintStmt();
        consume(TokenType::SEMICOLON, "Expect ';' after print statement.");
        return stmt;
    }

    if (check(TokenType::IDENTIFIER) && peek().lexeme == "println") {
        auto stmt = parsePrintlnStmt();
        consume(TokenType::SEMICOLON, "Expect ';' after print statement.");
        return stmt;
    }
    
    if(check(TokenType::KEYWORD) && peek().lexeme == "while"){
        return parseWhileStmt();
    }
    
    if (check(TokenType::KEYWORD) && peek().lexeme == "for") {
        return parseForStmt();
    }

    if (check(TokenType::LBRACE)) {
        return parseBlock();
    }

    if (check(TokenType::KEYWORD) && peek().lexeme == "if") {
        return parseIfStmt();
    }
    if (check(TokenType::KEYWORD) && peek().lexeme == "class") {
        return parseClassStmt();
    }

    if (check(TokenType::KEYWORD) && peek().lexeme == "trait") {
        return parseTraitStmt();
    }
    if (match(TokenType::KEYWORD) && previous().lexeme == "return") {
        std::unique_ptr<Expr> value = nullptr;

        if (!check(TokenType::SEMICOLON)) {
            value = parseExpression();
        }

        consume(TokenType::SEMICOLON, "Expect ';' after return");
        return std::make_unique<ReturnStmt>(std::move(value));
    }
    
    auto expr = parseExpression();
    TokenType opType = peek().type;
    if (isAssignmentOperator(opType)) {
        advance();
        std::vector<Assignment> assignments;
        auto value = parseExpression();
        
    
        assignments.emplace_back(std::move(expr), opType, std::move(value));
        
        while (match(TokenType::COMMA)) {
            auto nextTarget = parseExpression();
            
            TokenType nextOp = peek().type;
            if (nextOp == TokenType::EQUAL || nextOp == TokenType::PLUS_EQUAL ||
                nextOp == TokenType::SLASH_EQUAL || nextOp == TokenType::MINUS_EQUAL ||
                nextOp == TokenType::STAR_EQUAL || nextOp == TokenType::MOD_OP_EQUAL ||
                nextOp == TokenType::BITWISE_AND_EQUAL || nextOp == TokenType::BITWISE_OR_EQUAL ||
                nextOp == TokenType::XOR_EQUAL
                ) {
                advance();
            } else {
                throw std::runtime_error("Expect assignment operator."); 
            }
            
            auto nextValue = parseExpression();
            assignments.emplace_back(std::move(nextTarget), nextOp, std::move(nextValue));
        }
        consume(TokenType::SEMICOLON, "Expect ';' after assignment statement.");
        return std::make_unique<AssignmentStmt>(std::move(assignments));
    }
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<PrintStmt> Parser::parsePrintStmt() {
    
    advance(); 
    consume(TokenType::LPAREN, "Expect '(' after 'print'.");
    auto expr = parseExpression();
    consume(TokenType::RPAREN, "Expect ')' after print value.");
    return std::make_unique<PrintStmt>(std::move(expr));
}

std::unique_ptr<PrintlnStmt> Parser::parsePrintlnStmt(){
    advance(); 
    consume(TokenType::LPAREN, "Expect '(' after 'print'.");
    auto expr = parseExpression();
    consume(TokenType::RPAREN, "Expect ')' after print value.");
    return std::make_unique<PrintlnStmt>(std::move(expr));
}

std::unique_ptr<AssignmentStmt> Parser::parseAssignmentStmt() {
    
    std::vector<Assignment> assignments;
    
    do {
        Token nameToken = consume(TokenType::IDENTIFIER, "Expect variable name.");
        
        TokenType opType = peek().type;
        if (isAssignmentOperator(opType)) {
            advance(); 
        } else {
            throw std::runtime_error("Expect assignment operator after variable name.");
        }
        auto value = parseExpression();
        auto target = std::make_unique<VarExpr>(nameToken.lexeme);
        
        // Pass the operator type to Assignment
        assignments.emplace_back(std::move(target), opType, std::move(value));
        
    } while (match(TokenType::COMMA));
    
    return std::make_unique<AssignmentStmt>(std::move(assignments));
}

std::unique_ptr<ForStmt> Parser::parseForStmt() {
    advance(); 
    consume(TokenType::LPAREN, "Expect '(' after 'for'.");
    
    auto init = parseAssignmentStmt();
    consume(TokenType::SEMICOLON, "Expect ';' after loop initializer.");
    
    auto condition = parseExpression();
    consume(TokenType::SEMICOLON, "Expect ';' after loop condition.");
    
    auto increment = parseAssignmentStmt();
    consume(TokenType::RPAREN, "Expect ')' after loop clauses.");
    
    auto body = parseBlock();
    
    return std::make_unique<ForStmt>(std::move(init), std::move(condition), std::move(increment), std::move(body));
}

std::unique_ptr<WhileStmt> Parser::parseWhileStmt(){
    advance(); 
    consume(TokenType::LPAREN, "Expect '(' after 'while'.");

    auto condition = parseExpression();
    consume(TokenType::RPAREN, "Expect ')' after loop condition.");

    auto body = parseBlock();

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}
std::unique_ptr<IfStmt> Parser::parseIfStmt() {
    advance();
    consume(TokenType::LPAREN, "Expect '(' after 'if'.");
    auto condition = parseExpression();
    consume(TokenType::RPAREN, "Expect ')' after 'if' condition.");

    auto thenBranch = parseBlock();

    std::unique_ptr<Stmt> elseBranch = nullptr;


    if (check(TokenType::KEYWORD) && peek().lexeme == "else") {
        advance();

        if (check(TokenType::KEYWORD) && peek().lexeme == "if") {
            elseBranch = parseIfStmt(); 
        } else {
            elseBranch = parseBlock();
        }
    }

    return std::make_unique<IfStmt>(
        std::move(condition),
        std::move(thenBranch),
        std::move(elseBranch)
    );
}



std::unique_ptr<Expr> Parser::parseExpression() {
    return parseLogicalOr();
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();

    while (match(TokenType::OR)) { 
        std::string op = previous().lexeme;
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}


std::unique_ptr<Expr> Parser::parseBitwiseAnd(){
    auto left = parseEquality();
    while(match(TokenType::BITWISE_AND)) {
        std::string op = previous().lexeme;
        auto right = parseEquality();
        left = std::make_unique<BinaryExpr>(std::move(left),op,std::move(right));
    }
    return left;
}
std::unique_ptr<Expr> Parser::parseBitwiseXor(){
    auto left = parseBitwiseAnd();
    while(match(TokenType::BITWISE_XOR)){
        std::string op = previous().lexeme;
        auto right = parseBitwiseAnd();
        left = std::make_unique<BinaryExpr>(std::move(left),op,std::move(right));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseBitwiseOr(){
    auto left = parseBitwiseXor();
    while(match(TokenType::BITWISE_OR)){
        std::string op = previous().lexeme;
        auto right = parseBitwiseXor();
        left = std::make_unique<BinaryExpr>(std::move(left),op,std::move(right));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto left = parseBitwiseOr();

    while (match(TokenType::AND)) { 
        std::string op = previous().lexeme;
        auto right = parseBitwiseOr();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}
std::unique_ptr<Expr> Parser::parseEquality() {
    auto left = parseComparison();

    while (match(TokenType::EQUAL_EQUAL) || match(TokenType::NOT_EQUAL)) {
        std::string op = previous().lexeme;
        auto right = parseComparison();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}
std::unique_ptr<Expr> Parser::parseComparison() {
    auto left = parseShift();

    while (match(TokenType::LESS) || match(TokenType::LESS_EQUAL) ||
           match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL)) {
        std::string op = previous().lexeme;
        auto right = parseShift();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}
std::unique_ptr<Expr> Parser::parseShift() {
    auto left = parseAdditive();

    while (match(TokenType::LEFT_SHIFT) || match(TokenType::RIGHT_SHIFT)) {
        std::string op = previous().lexeme;
        auto right = parseAdditive();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseAdditive() {
    
    auto left = parseMultiplicative();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        std::string op = previous().lexeme;
        auto right = parseMultiplicative();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseMultiplicative() {
    auto left = parseUnary();

    while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::MOD_OP)) {
        std::string op = previous().lexeme;
        auto right = parseUnary();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (match(TokenType::NOT) || match(TokenType::MINUS)) {
        std::string op = previous().lexeme;
        auto right = parseUnary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }
    return parsePostfix();
}


std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();

    while (true) {
        if (match(TokenType::LBRACKET)) {
            auto index = parseExpression();
            consume(TokenType::RBRACKET, "Expect ']'.");
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
        }
        else if (match(TokenType::LPAREN)) {
            std::vector<std::unique_ptr<Expr>> arguments;
            if (!check(TokenType::RPAREN)) {
                do {
                    arguments.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            consume(TokenType::RPAREN, "Expect ')' after arguments.");
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(arguments));
        }
        else if (match(TokenType::DOT)) {
            Token name = consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
            expr = std::make_unique<MemberExpr>(std::move(expr), name.lexeme);
        }
        else {
            break;
        }
    }

    return expr;
}


std::unique_ptr<Expr> Parser::parsePrimary() {
    if (match(TokenType::LBRACKET)) {
        std::vector<std::unique_ptr<Expr>> elements;

        if (!check(TokenType::RBRACKET)) {
            do {
                elements.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        }

        consume(TokenType::RBRACKET, "Expect ']' after array literal.");
        return std::make_unique<ArrayExpr>(std::move(elements));
    }


    if (match(TokenType::NUMBER)) {
        return std::make_unique<NumberExpr>(previous().lexeme);
    }

    if (match(TokenType::STRING)) {
        return std::make_unique<StringExpr>(previous().lexeme);
    }
    
    if (match(TokenType::KEYWORD)) {
        if (previous().lexeme == "true") {
            return std::make_unique<BoolExpr>(true);
        }
        if (previous().lexeme == "false") {
            return std::make_unique<BoolExpr>(false);
        }
    }

    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<VarExpr>(previous().lexeme);
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expect ')' after expression.");
        return expr;
    }

    throw std::runtime_error("Expect expression.");
}

std::unique_ptr<TraitStmt> Parser::parseTraitStmt(){
    consume(TokenType::KEYWORD, "Expect 'trait' keyword.");
    Token name = consume(TokenType::IDENTIFIER, "Expect trait name.");
    std::vector<std::string> parents;
    if (check(TokenType::KEYWORD) && peek().lexeme == "inherits") {
        advance();
        do {
            Token parent = consume(TokenType::IDENTIFIER, "Expect trait name.");
            parents.push_back(parent.lexeme);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::LBRACE, "Expect '{' after trait name.");

    std::vector<std::unique_ptr<TraitSection>> sections;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        if (check(TokenType::KEYWORD) && (peek().lexeme == "private")){
            throw std::runtime_error("trait cannot have private feild");
        }
        if (check(TokenType::KEYWORD) && (peek().lexeme == "public" || peek().lexeme == "protected" || peek().lexeme == "shared")) {
            sections.push_back(parseTraitSection());
        } else {
            auto member = parseTraitMember();
            std::vector<std::unique_ptr<TraitMember>> members;
            members.push_back(std::move(member));
            sections.push_back(std::make_unique<TraitSection>(TraitAccessModifier::PUBLIC, std::move(members)));
        }
    }

    consume(TokenType::RBRACE, "Expect '}' after trait body.");
    return std::make_unique<TraitStmt>(name.lexeme, std::move(sections), std::move(parents));
}

std::unique_ptr<TraitSection> Parser::parseTraitSection() {
    TraitAccessModifier modifier = parseTraitAccessModifier();
    consume(TokenType::LBRACE, "Expect '{' after access modifier.");
    std::vector<std::unique_ptr<TraitMember>> members;

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        members.push_back(parseTraitMember());
    }

    consume(TokenType::RBRACE, "Expect '}' after section.");

    return std::make_unique<TraitSection>(modifier, std::move(members));
}
TraitAccessModifier Parser::parseTraitAccessModifier() {
    if (match(TokenType::KEYWORD)) {
        std::string word = previous().lexeme;

        if (word == "public") return TraitAccessModifier::PUBLIC;
        if (word == "protected") return TraitAccessModifier::PROTECTED;
        if (word == "shared") return TraitAccessModifier::SHARED;
    }

    throw std::runtime_error("Expected access modifier (public/protected/shared)");
}

std::unique_ptr<TraitMember> Parser::parseTraitMember() {
    if (check(TokenType::KEYWORD)) {
        std::string lexeme = peek().lexeme;
        
        if (lexeme == "dec") {
            advance(); 
            consume(TokenType::COLON, "Expect ':' after dec.");

            Token name = consume(TokenType::IDENTIFIER, "Expect identifier.");

            if (match(TokenType::LPAREN)) {
                std::vector<Param> params;
                if (!check(TokenType::RPAREN)) {
                    do {
                        bool isRef = false;
                        if (match(TokenType::KEYWORD) && previous().lexeme == "ref") {
                            consume(TokenType::COLON, "Expect ':' after ref.");
                            isRef = true;
                        }
                        Token paramName = consume(TokenType::IDENTIFIER, "Expect parameter name.");
                        params.emplace_back(paramName.lexeme, isRef);
                    } while (match(TokenType::COMMA));
                }
                consume(TokenType::RPAREN, "Expect ')' after parameters.");
                consume(TokenType::SEMICOLON, "Expect ';' after method declaration.");
                return std::make_unique<TraitMethodDecl>(name.lexeme, std::move(params));
            }

            std::unique_ptr<Expr> initializer = nullptr;
            if (match(TokenType::EQUAL)) {
                initializer = parseExpression();
            }

            consume(TokenType::SEMICOLON, "Expect ';' after field.");
            return std::make_unique<TraitFieldDecl>(name.lexeme, std::move(initializer));
        }
    }

    throw std::runtime_error("Invalid trait member. Found: " + peek().lexeme);
}


std::unique_ptr<ClassStmt> Parser::parseClassStmt() {
    consume(TokenType::KEYWORD, "Expect 'class' keyword.");
    Token name = consume(TokenType::IDENTIFIER, "Expect class name.");
    std::string parentName = "";
    if (check(TokenType::KEYWORD) && peek().lexeme == "inherits") {
        advance();
        Token parent = consume(TokenType::IDENTIFIER, "Expect class name.");
        parentName = parent.lexeme;
    }
    
    std::vector<std::string> impls;
    if (check(TokenType::KEYWORD) && peek().lexeme == "impl") {
        advance();
        do {
            Token traitName = consume(TokenType::IDENTIFIER, "Expect trait name.");
            impls.push_back(traitName.lexeme);
        } while (match(TokenType::COMMA));
    }
    

    consume(TokenType::LBRACE, "Expect '{' after class name.");

    std::vector<std::unique_ptr<ClassSection>> sections;

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        if (check(TokenType::KEYWORD) && (peek().lexeme == "public" || peek().lexeme == "private" || peek().lexeme == "protected" || peek().lexeme == "shared")) {
            sections.push_back(parseSection());
        } else {
            auto member = parseClassMember();
            std::vector<std::unique_ptr<ClassMember>> members;
            members.push_back(std::move(member));
            sections.push_back(std::make_unique<ClassSection>(AccessModifier::PUBLIC, std::move(members)));
        }
    }

    consume(TokenType::RBRACE, "Expect '}' after class body.");

    return std::make_unique<ClassStmt>(name.lexeme, std::move(sections), parentName, std::move(impls));
}

std::unique_ptr<ClassSection> Parser::parseSection() {
    AccessModifier modifier = parseAccessModifier();

    consume(TokenType::LBRACE, "Expect '{' after access modifier.");

    std::vector<std::unique_ptr<ClassMember>> members;

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        members.push_back(parseClassMember());
    }

    consume(TokenType::RBRACE, "Expect '}' after section.");

    return std::make_unique<ClassSection>(modifier, std::move(members));
}
AccessModifier Parser::parseAccessModifier() {
    if (match(TokenType::KEYWORD)) {
        std::string word = previous().lexeme;

        if (word == "public") return AccessModifier::PUBLIC;
        if (word == "private") return AccessModifier::PRIVATE;
        if (word == "protected") return AccessModifier::PROTECTED;
        if (word == "shared") return AccessModifier::SHARED;
    }

    throw std::runtime_error("Expected access modifier (public/private/protected/shared)");
}
std::unique_ptr<ClassMember> Parser::parseClassMember() {

    if (check(TokenType::KEYWORD)) {
        std::string lexeme = peek().lexeme;
        
        if (lexeme == "dec") {
            advance(); // Consume 'dec'
            consume(TokenType::COLON, "Expect ':' after dec.");

            Token name = consume(TokenType::IDENTIFIER, "Expect identifier.");

            // method declaration?
            if (match(TokenType::LPAREN)) {
                return parseMethodDeclAfterName(name.lexeme);
            }

            std::unique_ptr<Expr> initializer = nullptr;
            if (match(TokenType::EQUAL)) {
                initializer = parseExpression();
            }

            consume(TokenType::SEMICOLON, "Expect ';' after field.");
            return std::make_unique<FieldDecl>(name.lexeme, std::move(initializer));
        }
        
        if (lexeme == "func") {
            advance(); // Consume 'func'
            return parseMethodDef();
        }
    }

    throw std::runtime_error("Invalid class member. Found: " + peek().lexeme);
}
std::unique_ptr<MethodDecl> Parser::parseMethodDeclAfterName(std::string name) {

    std::vector<Param> params;

    if (!check(TokenType::RPAREN)) {
        do {
            bool isRef = false;

            if (match(TokenType::KEYWORD) && previous().lexeme == "ref") {
                consume(TokenType::COLON, "Expect ':' after ref.");
                isRef = true;
            }

            Token paramName = consume(TokenType::IDENTIFIER, "Expect parameter name.");
            params.emplace_back(paramName.lexeme, isRef);

        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RPAREN, "Expect ')' after parameters.");
    consume(TokenType::SEMICOLON, "Expect ';' after method declaration.");

    return std::make_unique<MethodDecl>(name, std::move(params));
}
std::unique_ptr<MethodDef> Parser::parseMethodDef() {
    Token name = consume(TokenType::IDENTIFIER, "Expect method name.");

    consume(TokenType::LPAREN, "Expect '(' after method name.");

    std::vector<Param> params;

    if (!check(TokenType::RPAREN)) {
        do {
            bool isRef = false;

            if (match(TokenType::KEYWORD) && previous().lexeme == "ref") {
                consume(TokenType::COLON, "Expect ':' after ref.");
                isRef = true;
            }

            Token paramName = consume(TokenType::IDENTIFIER, "Expect parameter name.");
            params.emplace_back(paramName.lexeme, isRef);

        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RPAREN, "Expect ')' after params.");

    auto body = parseBlock();

    return std::make_unique<MethodDef>(name.lexeme, std::move(params), std::move(body));
}




bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw std::runtime_error(message + " Found: " + peek().lexeme);
}
std::vector<Param> Parser::parseParams(){

    std::vector<Param> params;
    if (check(TokenType::RPAREN)) return params;
     do {
        bool isRef = false;

        
        if (check(TokenType::IDENTIFIER) && peek().lexeme == "ref") {
            advance(); 
            consume(TokenType::COLON, "Expected ':' after 'ref'");
            isRef = true;
        }

        consume(TokenType::IDENTIFIER, "Expected parameter name");
        std::string name = previous().lexeme;

        params.emplace_back(name, isRef);

    } while (match(TokenType::COMMA));

    return params;
}

std::unique_ptr<Function> Parser::parseFunction() {
    consume(TokenType::KEYWORD, "Expected 'func'");
    consume(TokenType::IDENTIFIER, "Expected function name");
    std::string name = previous().lexeme;

    consume(TokenType::LPAREN, "Expected '(' after function name");
    auto params = parseParams();
    consume(TokenType::RPAREN, "Expected ')' after parameters");

    auto body = parseBlock();

    return std::make_unique<Function>(name, std::move(params), std::move(body));
}

