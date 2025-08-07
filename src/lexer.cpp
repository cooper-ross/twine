#include "../include/lexer.h"
#include <iostream>
#include <stdexcept>

Lexer::Lexer(const std::string& src) : source(src), current(0), line(1), column(1) {
    initKeywords();
}

void Lexer::initKeywords() {
    keywords["let"] = TokenType::LET;
    keywords["var"] = TokenType::VAR;
    keywords["const"] = TokenType::CONST;
    keywords["function"] = TokenType::FUNCTION;
    keywords["if"] = TokenType::IF;
    keywords["else"] = TokenType::ELSE;
    keywords["while"] = TokenType::WHILE;
    keywords["for"] = TokenType::FOR;
    keywords["return"] = TokenType::RETURN;
    keywords["true"] = TokenType::TRUE;
    keywords["false"] = TokenType::FALSE;
    keywords["null"] = TokenType::NULL_TOKEN;
}

char Lexer::peek(int offset) const {
    size_t pos = current + offset;
    if (pos >= source.length()) return '\0';
    return source[pos];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    char c = source[current++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return current >= source.length();
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

Token Lexer::scanNumber() {
    int startLine = line;
    int startColumn = column;
    size_t start = current;
    
    advance(); // consume first digit
    while (isDigit(peek())) advance();
    
    // Handle decimal numbers
    if (peek() == '.' && isDigit(peek(1))) {
        advance(); // consume '.'
        while (isDigit(peek())) advance();
    }
    
    std::string value = source.substr(start, current - start);
    return Token(TokenType::NUMBER, value, startLine, startColumn);
}

Token Lexer::scanString() {
    int startLine = line;
    int startColumn = column;
    char quote = source[current];
    advance(); // consume opening quote
    std::string value;
    
    while (peek() != quote && !isAtEnd()) {
        if (peek() == '\\') {
            advance();
            char next = advance();
            switch (next) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '\'': value += '\''; break;
                default: value += next; break;
            }
        } else {
            value += advance();
        }
    }
    
    if (isAtEnd()) {
        error("Unterminated string", startLine, startColumn);
        return Token(TokenType::UNKNOWN, "", startLine, startColumn);
    }
    
    advance(); // closing quote
    return Token(TokenType::STRING, value, startLine, startColumn);
}

Token Lexer::scanIdentifier() {
    int startLine = line;
    int startColumn = column;
    size_t start = current;
    
    advance(); // consume first character
    while (isAlphaNumeric(peek())) advance();
    
    std::string value = source.substr(start, current - start);
    
    auto it = keywords.find(value);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
    
    return Token(type, value, startLine, startColumn);
}

void Lexer::skipWhitespace() {
    while (true) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipLineComment() {
    while (peek() != '\n' && !isAtEnd()) advance();
}

void Lexer::skipBlockComment() {
    while (true) {
        if (isAtEnd()) {
            error("Unterminated block comment", line, column);
            break;
        }
        if (peek() == '*' && peek(1) == '/') {
            advance(); // *
            advance(); // /
            break;
        }
        advance();
    }
}

Token Lexer::nextToken() {
    skipWhitespace();
    
    if (isAtEnd()) {
        return Token(TokenType::END_OF_FILE, "", line, column);
    }
    
    int startLine = line;
    int startColumn = column;
    char c = advance();
    
    // Numbers
    if (isDigit(c)) {
        // Move back to the digit for scanNumber to process
        current--;
        if (column > 1) column--;
        return scanNumber();
    }
    
    // Identifiers and keywords
    if (isAlpha(c)) {
        // Move back to the first letter for scanIdentifier to process
        current--;
        if (column > 1) column--;
        return scanIdentifier();
    }
    
    // Two-character operators and comments
    switch (c) {
        case '=':
            if (peek() == '=') {
                advance();
                return Token(TokenType::EQUAL, "==", startLine, startColumn);
            }
            return Token(TokenType::ASSIGN, "=", startLine, startColumn);
            
        case '!':
            if (peek() == '=') {
                advance();
                return Token(TokenType::NOT_EQUAL, "!=", startLine, startColumn);
            }
            return Token(TokenType::LOGICAL_NOT, "!", startLine, startColumn);
            
        case '<':
            if (peek() == '=') {
                advance();
                return Token(TokenType::LESS_EQUAL, "<=", startLine, startColumn);
            }
            return Token(TokenType::LESS_THAN, "<", startLine, startColumn);
            
        case '>':
            if (peek() == '=') {
                advance();
                return Token(TokenType::GREATER_EQUAL, ">=", startLine, startColumn);
            }
            return Token(TokenType::GREATER_THAN, ">", startLine, startColumn);
            
        case '&':
            if (peek() == '&') {
                advance();
                return Token(TokenType::LOGICAL_AND, "&&", startLine, startColumn);
            }
            break;
            
        case '|':
            if (peek() == '|') {
                advance();
                return Token(TokenType::LOGICAL_OR, "||", startLine, startColumn);
            }
            break;
            
        case '/':
            if (peek() == '/') {
                skipLineComment();
                return nextToken();
            }
            if (peek() == '*') {
                advance();
                skipBlockComment();
                return nextToken();
            }
            return Token(TokenType::DIVIDE, "/", startLine, startColumn);
            
        // Single-character tokens
        case '+': return Token(TokenType::PLUS, "+", startLine, startColumn);
        case '-': return Token(TokenType::MINUS, "-", startLine, startColumn);
        case '*': return Token(TokenType::MULTIPLY, "*", startLine, startColumn);
        case '%': return Token(TokenType::MODULO, "%", startLine, startColumn);
        case ';': return Token(TokenType::SEMICOLON, ";", startLine, startColumn);
        case ',': return Token(TokenType::COMMA, ",", startLine, startColumn);
        case '.': return Token(TokenType::DOT, ".", startLine, startColumn);
        case '(': return Token(TokenType::LEFT_PAREN, "(", startLine, startColumn);
        case ')': return Token(TokenType::RIGHT_PAREN, ")", startLine, startColumn);
        case '{': return Token(TokenType::LEFT_BRACE, "{", startLine, startColumn);
        case '}': return Token(TokenType::RIGHT_BRACE, "}", startLine, startColumn);
        case '[': return Token(TokenType::LEFT_BRACKET, "[", startLine, startColumn);
        case ']': return Token(TokenType::RIGHT_BRACKET, "]", startLine, startColumn);
        
        // String literals
        case '"':
        case '\'':
            // Move back to the quote for scanString to process
            current--;
            if (column > 1) column--;
            return scanString();
    }
    
    error(std::string("Unexpected character: ") + c, startLine, startColumn);
    return Token(TokenType::UNKNOWN, std::string(1, c), startLine, startColumn);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    Token token;
    
    do {
        token = nextToken();
        tokens.push_back(token);
    } while (token.type != TokenType::END_OF_FILE);
    
    return tokens;
}

void Lexer::error(const std::string& message, int line, int column) {
    std::cerr << "Lexer Error at line " << line << ", column " << column << ": " << message << std::endl;
}