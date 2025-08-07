#include "../include/ast.h"

void Program::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void NumberLiteral::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void StringLiteral::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void BooleanLiteral::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void NullLiteral::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void Identifier::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void BinaryExpression::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void UnaryExpression::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void AssignmentExpression::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void CallExpression::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void ExpressionStatement::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void VariableDeclaration::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void BlockStatement::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void IfStatement::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void WhileStatement::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void ForStatement::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void ReturnStatement::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

void FunctionDeclaration::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}