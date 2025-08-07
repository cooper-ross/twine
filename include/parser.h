#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include <memory>
#include <vector>
#include <string>
#include <stdexcept>

class Parser {
private:
    std::vector<Token> tokens;
    size_t current;
    
    Token peek(int offset = 0) const;
    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    void synchronize();
    
    // Error handling
    class ParseError : public std::runtime_error {
    public:
        explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
    };
    
    ParseError error(const Token& token, const std::string& message);
    void reportError(const std::string& message, const Token& token);
    
    // Parsing methods (in order of precedence)
    std::unique_ptr<Program> parseProgram();
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Statement> parseDeclaration();
    std::unique_ptr<Statement> parseVariableDeclaration();
    std::unique_ptr<Statement> parseFunctionDeclaration();
    std::unique_ptr<Statement> parseIfStatement();
    std::unique_ptr<Statement> parseWhileStatement();
    std::unique_ptr<Statement> parseForStatement();
    std::unique_ptr<Statement> parseReturnStatement();
    std::unique_ptr<Statement> parseBlockStatement();
    std::unique_ptr<Statement> parseExpressionStatement();
    
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseAssignment();
    std::unique_ptr<Expression> parseLogicalOr();
    std::unique_ptr<Expression> parseLogicalAnd();
    std::unique_ptr<Expression> parseEquality();
    std::unique_ptr<Expression> parseComparison();
    std::unique_ptr<Expression> parseAddition();
    std::unique_ptr<Expression> parseMultiplication();
    std::unique_ptr<Expression> parseUnary();
    std::unique_ptr<Expression> parseCall();
    std::unique_ptr<Expression> parsePrimary();
    
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::unique_ptr<Program> parse();
};

#endif // PARSER_H