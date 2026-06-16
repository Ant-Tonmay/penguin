#pragma once

#include <string>
#include <vector>
#include <unordered_map>

enum class TokenType {
    IDENTIFIER,
    NUMBER,
    PLUS,
    PLUS_EQUAL,
    MINUS,
    MINUS_EQUAL,
    STAR,
    STAR_EQUAL,
    SLASH,
    SLASH_EQUAL,
    MOD_OP,
    MOD_OP_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    NOT_EQUAL,
    BITWISE_AND,
    BITWISE_AND_EQUAL,
    BITWISE_OR,
    BITWISE_OR_EQUAL,
    BITWISE_XOR,
    XOR_EQUAL,
    OR,
    AND,
    NOT,
    LEFT_SHIFT,
    RIGHT_SHIFT,
    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    LBRACE,
    RBRACE,
    SEMICOLON,
    COMMA,
    COLON,
    STRING,
    CHAR,
    DOT,
    KEYWORD,
    EOF_TOKEN
};

struct SourceLocation {
    std::string line;
    int line_num;
    int col_num;
    
};

struct Token {
    TokenType type;
    std::string lexeme;
    SourceLocation location;

    Token(TokenType type, const std::string& lexeme, SourceLocation location);
    Token(TokenType type, const std::string& lexeme);
    std::string toString() const;
};


class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source;
    std::vector<Token> tokens;
    size_t current;
    int line_num = 1;
    int col_num = 1;
    size_t line_start = 0;
    
    bool isAtEnd() const;
    char advance();
    char peek() const;

    void addToken(TokenType type, const std::string& lexeme);
    void number();
    void identifier();
    void _string();
    void _char();
};
