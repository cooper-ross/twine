#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include <variant>

// Forward declarations
class ASTVisitor;

// Base AST Node
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor* visitor) = 0;
};

// Expression nodes
class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

class NumberLiteral : public Expression {
public:
    double value;
    
    explicit NumberLiteral(double v) : value(v) {}
    void accept(ASTVisitor* visitor) override;
};

class StringLiteral : public Expression {
public:
    std::string value;
    
    explicit StringLiteral(const std::string& v) : value(v) {}
    void accept(ASTVisitor* visitor) override;
};

class BooleanLiteral : public Expression {
public:
    bool value;
    
    explicit BooleanLiteral(bool v) : value(v) {}
    void accept(ASTVisitor* visitor) override;
};

class NullLiteral : public Expression {
public:
    void accept(ASTVisitor* visitor) override;
};

class Identifier : public Expression {
public:
    std::string name;
    
    explicit Identifier(const std::string& n) : name(n) {}
    void accept(ASTVisitor* visitor) override;
};

class BinaryExpression : public Expression {
public:
    std::unique_ptr<Expression> left;
    std::string op;
    std::unique_ptr<Expression> right;
    
    BinaryExpression(std::unique_ptr<Expression> l, const std::string& o, std::unique_ptr<Expression> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
    void accept(ASTVisitor* visitor) override;
};

class UnaryExpression : public Expression {
public:
    std::string op;
    std::unique_ptr<Expression> operand;
    
    UnaryExpression(const std::string& o, std::unique_ptr<Expression> expr)
        : op(o), operand(std::move(expr)) {}
    void accept(ASTVisitor* visitor) override;
};

class AssignmentExpression : public Expression {
public:
    std::string name;
    std::unique_ptr<Expression> value;
    
    AssignmentExpression(const std::string& n, std::unique_ptr<Expression> v)
        : name(n), value(std::move(v)) {}
    void accept(ASTVisitor* visitor) override;
};

class IndexAssignmentExpression : public Expression {
public:
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;
    std::unique_ptr<Expression> value;
    
    IndexAssignmentExpression(std::unique_ptr<Expression> arr, 
                              std::unique_ptr<Expression> idx,
                              std::unique_ptr<Expression> val)
        : array(std::move(arr)), index(std::move(idx)), value(std::move(val)) {}
    void accept(ASTVisitor* visitor) override;
};

class CallExpression : public Expression {
public:
    std::string name;
    std::vector<std::unique_ptr<Expression>> arguments;
    
    CallExpression(const std::string& n, std::vector<std::unique_ptr<Expression>> args)
        : name(n), arguments(std::move(args)) {}
    void accept(ASTVisitor* visitor) override;
};

class ArrayLiteral : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> elements;
    
    explicit ArrayLiteral(std::vector<std::unique_ptr<Expression>> elems)
        : elements(std::move(elems)) {}
    void accept(ASTVisitor* visitor) override;
};

class IndexExpression : public Expression {
public:
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;
    
    IndexExpression(std::unique_ptr<Expression> arr, std::unique_ptr<Expression> idx)
        : array(std::move(arr)), index(std::move(idx)) {}
    void accept(ASTVisitor* visitor) override;
};

// Statement nodes
class Statement : public ASTNode {
public:
    virtual ~Statement() = default;
};

class ExpressionStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;
    
    explicit ExpressionStatement(std::unique_ptr<Expression> expr)
        : expression(std::move(expr)) {}
    void accept(ASTVisitor* visitor) override;
};

class VariableDeclaration : public Statement {
public:
    std::string kind; // "let", "var", or "const"
    std::string name;
    std::unique_ptr<Expression> initializer;
    
    VariableDeclaration(const std::string& k, const std::string& n, std::unique_ptr<Expression> init = nullptr)
        : kind(k), name(n), initializer(std::move(init)) {}
    void accept(ASTVisitor* visitor) override;
};

class BlockStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    
    explicit BlockStatement(std::vector<std::unique_ptr<Statement>> stmts)
        : statements(std::move(stmts)) {}
    void accept(ASTVisitor* visitor) override;
};

class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenStatement;
    std::unique_ptr<Statement> elseStatement;
    
    IfStatement(std::unique_ptr<Expression> cond, 
                std::unique_ptr<Statement> thenStmt,
                std::unique_ptr<Statement> elseStmt = nullptr)
        : condition(std::move(cond)), 
          thenStatement(std::move(thenStmt)), 
          elseStatement(std::move(elseStmt)) {}
    void accept(ASTVisitor* visitor) override;
};

class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    
    WhileStatement(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> b)
        : condition(std::move(cond)), body(std::move(b)) {}
    void accept(ASTVisitor* visitor) override;
};

class ForStatement : public Statement {
public:
    std::unique_ptr<Statement> init;
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> update;
    std::unique_ptr<Statement> body;
    
    ForStatement(std::unique_ptr<Statement> i,
                 std::unique_ptr<Expression> c,
                 std::unique_ptr<Expression> u,
                 std::unique_ptr<Statement> b)
        : init(std::move(i)), condition(std::move(c)), 
          update(std::move(u)), body(std::move(b)) {}
    void accept(ASTVisitor* visitor) override;
};

class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> value;
    
    explicit ReturnStatement(std::unique_ptr<Expression> v = nullptr)
        : value(std::move(v)) {}
    void accept(ASTVisitor* visitor) override;
};

class FunctionDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::string> parameters;
    std::unique_ptr<BlockStatement> body;
    
    FunctionDeclaration(const std::string& n,
                        std::vector<std::string> params,
                        std::unique_ptr<BlockStatement> b)
        : name(n), parameters(std::move(params)), body(std::move(b)) {}
    void accept(ASTVisitor* visitor) override;
};

// Program node (root of AST)
class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    
    explicit Program(std::vector<std::unique_ptr<Statement>> stmts)
        : statements(std::move(stmts)) {}
    void accept(ASTVisitor* visitor) override;
};

// Visitor pattern for AST traversal
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    virtual void visit(Program* node) = 0;
    virtual void visit(NumberLiteral* node) = 0;
    virtual void visit(StringLiteral* node) = 0;
    virtual void visit(BooleanLiteral* node) = 0;
    virtual void visit(NullLiteral* node) = 0;
    virtual void visit(Identifier* node) = 0;
    virtual void visit(BinaryExpression* node) = 0;
    virtual void visit(UnaryExpression* node) = 0;
    virtual void visit(AssignmentExpression* node) = 0;
    virtual void visit(IndexAssignmentExpression* node) = 0;
    virtual void visit(CallExpression* node) = 0;
    virtual void visit(ArrayLiteral* node) = 0;
    virtual void visit(IndexExpression* node) = 0;
    virtual void visit(ExpressionStatement* node) = 0;
    virtual void visit(VariableDeclaration* node) = 0;
    virtual void visit(BlockStatement* node) = 0;
    virtual void visit(IfStatement* node) = 0;
    virtual void visit(WhileStatement* node) = 0;
    virtual void visit(ForStatement* node) = 0;
    virtual void visit(ReturnStatement* node) = 0;
    virtual void visit(FunctionDeclaration* node) = 0;
};

#endif // AST_H