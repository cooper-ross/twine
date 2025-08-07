#include "../include/parser.h"
#include <iostream>
#include <sstream>

Parser::Parser(const std::vector<Token>& toks) : tokens(toks), current(0) {}

Token Parser::peek(int offset) const {
    size_t pos = current + offset;
    if (pos >= tokens.size()) {
        return tokens.back(); // EOF token
    }
    return tokens[pos];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw error(peek(), message);
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (tokens[current - 1].type == TokenType::SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::FUNCTION:
            case TokenType::VAR:
            case TokenType::LET:
            case TokenType::CONST:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
                return;
            default:
                break;
        }
        
        advance();
    }
}

Parser::ParseError Parser::error(const Token& token, const std::string& message) {
    reportError(message, token);
    return ParseError(message);
}

void Parser::reportError(const std::string& message, const Token& token) {
    std::cerr << "Parse Error at line " << token.line << ", column " << token.column;
    if (token.type == TokenType::END_OF_FILE) {
        std::cerr << " at end of file";
    } else {
        std::cerr << " at '" << token.value << "'";
    }
    std::cerr << ": " << message << std::endl;
}

std::unique_ptr<Program> Parser::parse() {
    try {
        return parseProgram();
    } catch (const ParseError& e) {
        return nullptr;
    }
}

std::unique_ptr<Program> Parser::parseProgram() {
    std::vector<std::unique_ptr<Statement>> statements;
    
    while (!isAtEnd()) {
        try {
            statements.push_back(parseStatement());
        } catch (const ParseError& e) {
            synchronize();
        }
    }
    
    return std::make_unique<Program>(std::move(statements));
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (match(TokenType::FUNCTION)) return parseFunctionDeclaration();
    if (match({TokenType::VAR, TokenType::LET, TokenType::CONST})) {
        Token kind = tokens[current - 1];
        return parseVariableDeclaration();
    }
    if (match(TokenType::IF)) return parseIfStatement();
    if (match(TokenType::WHILE)) return parseWhileStatement();
    if (match(TokenType::FOR)) return parseForStatement();
    if (match(TokenType::RETURN)) return parseReturnStatement();
    if (match(TokenType::LEFT_BRACE)) return parseBlockStatement();
    
    return parseExpressionStatement();
}

std::unique_ptr<Statement> Parser::parseVariableDeclaration() {
    Token kind = tokens[current - 1];
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name");
    
    std::unique_ptr<Expression> initializer = nullptr;
    if (match(TokenType::ASSIGN)) {
        initializer = parseExpression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    return std::make_unique<VariableDeclaration>(kind.value, name.value, std::move(initializer));
}

std::unique_ptr<Statement> Parser::parseFunctionDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    
    consume(TokenType::LEFT_PAREN, "Expected '(' after function name");
    std::vector<std::string> parameters;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            Token param = consume(TokenType::IDENTIFIER, "Expected parameter name");
            parameters.push_back(param.value);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    
    consume(TokenType::LEFT_BRACE, "Expected '{' before function body");
    auto body = parseBlockStatement();
    
    return std::make_unique<FunctionDeclaration>(
        name.value, 
        std::move(parameters), 
        std::unique_ptr<BlockStatement>(static_cast<BlockStatement*>(body.release()))
    );
}

std::unique_ptr<Statement> Parser::parseIfStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'if'");
    auto condition = parseExpression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after if condition");
    
    auto thenStatement = parseStatement();
    std::unique_ptr<Statement> elseStatement = nullptr;
    
    if (match(TokenType::ELSE)) {
        elseStatement = parseStatement();
    }
    
    return std::make_unique<IfStatement>(
        std::move(condition), 
        std::move(thenStatement), 
        std::move(elseStatement)
    );
}

std::unique_ptr<Statement> Parser::parseWhileStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'while'");
    auto condition = parseExpression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after while condition");
    
    auto body = parseStatement();
    
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::parseForStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'for'");
    
    std::unique_ptr<Statement> init = nullptr;
    if (match(TokenType::SEMICOLON)) {
        // No initializer
    } else if (match({TokenType::VAR, TokenType::LET, TokenType::CONST})) {
        init = parseVariableDeclaration();
    } else {
        auto expr = parseExpression();
        consume(TokenType::SEMICOLON, "Expected ';' after for loop initializer");
        init = std::make_unique<ExpressionStatement>(std::move(expr));
    }
    
    std::unique_ptr<Expression> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after for loop condition");
    
    std::unique_ptr<Expression> update = nullptr;
    if (!check(TokenType::RIGHT_PAREN)) {
        update = parseExpression();
    }
    consume(TokenType::RIGHT_PAREN, "Expected ')' after for clauses");
    
    auto body = parseStatement();
    
    return std::make_unique<ForStatement>(
        std::move(init), 
        std::move(condition), 
        std::move(update), 
        std::move(body)
    );
}

std::unique_ptr<Statement> Parser::parseReturnStatement() {
    std::unique_ptr<Expression> value = nullptr;
    
    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after return value");
    return std::make_unique<ReturnStatement>(std::move(value));
}

std::unique_ptr<Statement> Parser::parseBlockStatement() {
    std::vector<std::unique_ptr<Statement>> statements;
    
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        statements.push_back(parseStatement());
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after block");
    return std::make_unique<BlockStatement>(std::move(statements));
}

std::unique_ptr<Statement> Parser::parseExpressionStatement() {
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression");
    return std::make_unique<ExpressionStatement>(std::move(expr));
}

std::unique_ptr<Expression> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<Expression> Parser::parseAssignment() {
    auto expr = parseLogicalOr();
    
    if (match(TokenType::ASSIGN)) {
        if (auto* id = dynamic_cast<Identifier*>(expr.get())) {
            auto value = parseAssignment();
            return std::make_unique<AssignmentExpression>(id->name, std::move(value));
        }
        error(tokens[current - 1], "Invalid assignment target");
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    
    while (match(TokenType::LOGICAL_OR)) {
        std::string op = tokens[current - 1].value;
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseLogicalAnd() {
    auto expr = parseEquality();
    
    while (match(TokenType::LOGICAL_AND)) {
        std::string op = tokens[current - 1].value;
        auto right = parseEquality();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseEquality() {
    auto expr = parseComparison();
    
    while (match({TokenType::EQUAL, TokenType::NOT_EQUAL})) {
        std::string op = tokens[current - 1].value;
        auto right = parseComparison();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseComparison() {
    auto expr = parseAddition();
    
    while (match({TokenType::GREATER_THAN, TokenType::GREATER_EQUAL, 
                   TokenType::LESS_THAN, TokenType::LESS_EQUAL})) {
        std::string op = tokens[current - 1].value;
        auto right = parseAddition();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseAddition() {
    auto expr = parseMultiplication();
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        std::string op = tokens[current - 1].value;
        auto right = parseMultiplication();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseMultiplication() {
    auto expr = parseUnary();
    
    while (match({TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO})) {
        std::string op = tokens[current - 1].value;
        auto right = parseUnary();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseUnary() {
    if (match({TokenType::LOGICAL_NOT, TokenType::MINUS})) {
        std::string op = tokens[current - 1].value;
        auto right = parseUnary();
        return std::make_unique<UnaryExpression>(op, std::move(right));
    }
    
    return parseCall();
}

std::unique_ptr<Expression> Parser::parseCall() {
    auto expr = parsePrimary();
    
    while (true) {
        if (match(TokenType::LEFT_PAREN)) {
            if (auto* id = dynamic_cast<Identifier*>(expr.get())) {
                std::vector<std::unique_ptr<Expression>> arguments;
                
                if (!check(TokenType::RIGHT_PAREN)) {
                    do {
                        arguments.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                }
                
                consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments");
                expr = std::make_unique<CallExpression>(id->name, std::move(arguments));
            } else {
                error(tokens[current - 1], "Can only call functions");
            }
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    if (match(TokenType::TRUE)) {
        return std::make_unique<BooleanLiteral>(true);
    }
    
    if (match(TokenType::FALSE)) {
        return std::make_unique<BooleanLiteral>(false);
    }
    
    if (match(TokenType::NULL_TOKEN)) {
        return std::make_unique<NullLiteral>();
    }
    
    if (match(TokenType::NUMBER)) {
        double value = std::stod(tokens[current - 1].value);
        return std::make_unique<NumberLiteral>(value);
    }
    
    if (match(TokenType::STRING)) {
        return std::make_unique<StringLiteral>(tokens[current - 1].value);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<Identifier>(tokens[current - 1].value);
    }
    
    if (match(TokenType::LEFT_PAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }
    
    throw error(peek(), "Expected expression");
}