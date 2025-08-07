#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

enum class TokenType {
    // Literals
    NUMBER,
    STRING,
    IDENTIFIER,
    
    // Keywords
    LET,
    VAR,
    CONST,
    FUNCTION,
    IF,
    ELSE,
    WHILE,
    FOR,
    RETURN,
    TRUE,
    FALSE,
    NULL_TOKEN,
    
    // Operators
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULO,
    ASSIGN,
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    GREATER_THAN,
    LESS_EQUAL,
    GREATER_EQUAL,
    LOGICAL_AND,
    LOGICAL_OR,
    LOGICAL_NOT,
    
    // Punctuation
    SEMICOLON,
    COMMA,
    DOT,
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    
    // Special
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType t = TokenType::UNKNOWN, const std::string& v = "", int l = 0, int c = 0)
        : type(t), value(v), line(l), column(c) {}
};

class Lexer {
private:
    std::string source;
    size_t current;
    int line;
    int column;
    std::unordered_map<std::string, TokenType> keywords;
    
    void initKeywords();
    char peek(int offset = 0) const;
    char advance();
    bool isAtEnd() const;
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    
    Token scanNumber();
    Token scanString();
    Token scanIdentifier();
    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();
    
public:
    Lexer(const std::string& src);
    Token nextToken();
    std::vector<Token> tokenize();
    
    // Error reporting
    void error(const std::string& message, int line, int column);
};

#endif // LEXER_H