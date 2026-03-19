#include "lexer/lexer.h"
#include <iostream>
#include <cctype>
#include <vector>
#include <string>
#include <unordered_map>



Token::Token(TokenType type, const std::string& lexeme)
    : type(type), lexeme(lexeme) {}

std::string Token::toString() const {
    std::string typeStr;
    switch (type) {
        case TokenType::IDENTIFIER: typeStr = "IDENTIFIER"; break;
        case TokenType::NUMBER: typeStr = "NUMBER"; break;
        case TokenType::PLUS: typeStr = "PLUS"; break;
        case TokenType::MINUS: typeStr = "MINUS"; break;
        case TokenType::STAR: typeStr = "STAR"; break;
        case TokenType::SLASH: typeStr = "SLASH"; break;
        case TokenType::EQUAL: typeStr = "EQUAL"; break;
        case TokenType::EQUAL_EQUAL: typeStr = "EQUAL_EQUAL"; break;
        case TokenType::LESS: typeStr = "LESS"; break;
        case TokenType::GREATER: typeStr = "GREATER"; break;
        case TokenType::LPAREN: typeStr = "LPAREN"; break;
        case TokenType::RPAREN: typeStr = "RPAREN"; break;
        case TokenType::LBRACKET: typeStr = "LBRACKET"; break;
        case TokenType::RBRACKET: typeStr = "RBRACKET"; break;
        case TokenType::LBRACE: typeStr = "LBRACE"; break;
        case TokenType::RBRACE: typeStr = "RBRACE"; break;
        case TokenType::SEMICOLON: typeStr = "SEMICOLON"; break;
        case TokenType::COMMA: typeStr = "COMMA"; break;
        case TokenType::STRING: typeStr = "STRING"; break;
        case TokenType::KEYWORD: typeStr = "KEYWORD"; break;
        case TokenType::EOF_TOKEN: typeStr = "EOF"; break;
        default: typeStr = "UNKNOWN"; break;
    }
    return "Token(" + typeStr + ", \"" + lexeme + "\")";
}

Lexer::Lexer(const std::string& src)
    : source(src), current(0) {}




bool Lexer::isAtEnd() const {
    return current >= source.size();
}
char Lexer::advance() {
    return source[current++];
}
char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}
void Lexer::addToken(TokenType type, const std::string& lexeme) {
    tokens.emplace_back(type, lexeme);
}

void Lexer::number() {
    size_t start = current - 1;
    while (isdigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && isdigit(source[current + 1])) {
        // Consume the "."
        advance();

        while (isdigit(peek())) advance();
    }

    std::string value = source.substr(start, current - start);
    addToken(TokenType::NUMBER, value);
}

void Lexer::string() {
    size_t start = current;
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            // Handle newlines if tracked
        }
        advance();
    }

    if (isAtEnd()) {
        std::cerr << "Unterminated string." << std::endl;
        return;
    }

    std::string value = source.substr(start, current - start);
    advance(); // Consume the closing "
    addToken(TokenType::STRING, value);
}

void Lexer::identifier() {
    size_t start = current - 1;
    while (isalnum(peek()) || peek() == '_') advance();

    std::string value = source.substr(start, current - start);
    
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"if", TokenType::KEYWORD},
        {"else", TokenType::KEYWORD},
        {"while", TokenType::KEYWORD},
        {"return", TokenType::KEYWORD},
        {"func", TokenType::KEYWORD},
        {"true", TokenType::KEYWORD},
        {"false", TokenType::KEYWORD},
        {"break", TokenType::KEYWORD},
        {"continue", TokenType::KEYWORD},
        {"for",TokenType::KEYWORD},
        {"class",TokenType::KEYWORD},
        {"dec",TokenType::KEYWORD},
        {"private",TokenType::KEYWORD},
        {"public",TokenType::KEYWORD},
        {"protected",TokenType::KEYWORD},
        {"inherits",TokenType::KEYWORD},
        {"shared",TokenType::KEYWORD},
        {"trait",TokenType::KEYWORD},
        {"impl",TokenType::KEYWORD},
        
    };

    if (keywords.count(value)) {
        addToken(TokenType::KEYWORD, value);
    } else {
        addToken(TokenType::IDENTIFIER, value);
    }
}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        char c = advance();

        switch (c) {
            case '+': 
                if(peek() =='='){
                    advance();
                    addToken(TokenType::PLUS_EQUAL,"+=");
                }else{
                    addToken(TokenType::PLUS,"+");
                }
                break;
            case '-':
                if(peek() =='='){
                    advance();
                    addToken(TokenType::MINUS_EQUAL,"-=");
                }else{
                    addToken(TokenType::MINUS,"-");
                }
                break;
            case '*': 
                if(peek() == '='){
                    advance();
                    addToken(TokenType::STAR_EQUAL,"*=");
                }else{
                    addToken(TokenType::STAR, "*");
                }
                break;
            case '%':
                if(peek() == '='){
                        advance();
                        addToken(TokenType::MOD_OP_EQUAL,"%=");
                }else{
                        addToken(TokenType::MOD_OP, "%");
                }
                break; 
            case '/': 
                if (peek() == '/') {
                    while (peek() != '\n' && !isAtEnd()) advance();
                }else if(peek() == '='){
                    advance();
                    addToken(TokenType::SLASH_EQUAL,"/=");
                }else {
                    addToken(TokenType::SLASH, "/");
                }
                break;
            case '&' :
                if(peek() == '&'){
                    advance();
                    addToken(TokenType::AND, "&&");
                }else if(peek() == '='){
                    advance();
                    addToken(TokenType::BITWISE_AND_EQUAL, "&=");
                }else{
                    addToken(TokenType::BITWISE_AND, "&");
                }
                break;
            case '|' :
                if(peek() == '|'){
                    advance();
                    addToken(TokenType::OR, "||");
                }else if(peek() == '='){
                    advance();
                    addToken(TokenType::BITWISE_OR_EQUAL, "|=");
                }else{
                    addToken(TokenType::BITWISE_OR, "|");
                }
                break;
            case '^' :
                if(peek() =='='){
                    advance();
                    addToken(TokenType::XOR_EQUAL,"^=");
                }else{
                    addToken(TokenType::BITWISE_XOR, "^");
                }
                break;
            case '!' :
                if(peek() == '='){
                    advance();
                    addToken(TokenType::NOT_EQUAL, "!=");
                }else{
                    addToken(TokenType::NOT, "!");
                }
                break;
            case '=':
                if (peek() == '=') {
                    advance();
                    addToken(TokenType::EQUAL_EQUAL, "==");
                } else {
                    addToken(TokenType::EQUAL, "=");
                }
                break;
            case '<': 
                if(peek() =='='){
                    advance();
                    addToken(TokenType::LESS_EQUAL, "<=");
                }else if (peek() == '<'){
                    advance();
                    addToken(TokenType::LEFT_SHIFT, "<<");
                }else{
                    addToken(TokenType::LESS, "<");
                }
                break;
            case '>': 
                if(peek() == '='){
                    advance();  
                    addToken(TokenType::GREATER_EQUAL, ">=");
                }else if(peek() == '>'){
                    advance();
                    addToken(TokenType::RIGHT_SHIFT, ">>");
                }else{
                    addToken(TokenType::GREATER, ">");
                }
                break;
            case '(': addToken(TokenType::LPAREN, "("); break;
            case ')': addToken(TokenType::RPAREN, ")"); break;
            case '[': addToken(TokenType::LBRACKET, "["); break;
            case ']': addToken(TokenType::RBRACKET, "]"); break;
            case '{': addToken(TokenType::LBRACE, "{"); break;
            case '}': addToken(TokenType::RBRACE, "}"); break;
            case ';': addToken(TokenType::SEMICOLON, ";"); break;
            case ':': addToken(TokenType::COLON, ":"); break;
            case ',': addToken(TokenType::COMMA, ","); break;
            case '.': addToken(TokenType::DOT, "."); break;
            case '"': string(); break;

            case ' ':
            case '\t':
            case '\n':
            case '\r':
                break; // ignore whitespace

            default:
                if (isdigit(c)) {
                    number();
                } else if (isalpha(c) || c == '_') {
                    identifier();
                }  else {
                    std::cerr << "Unexpected character: " << c << "\n";
                }
            }
        }

    addToken(TokenType::EOF_TOKEN, "");
    return tokens;
}

