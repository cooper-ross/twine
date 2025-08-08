#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <stack>

class CodeGenerator : public ASTVisitor {
private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    
    // Symbol tables for variables and functions
    std::vector<std::map<std::string, llvm::AllocaInst*>> symbolTables;
    std::map<std::string, llvm::Function*> functions;
    
    // Current function being generated
    llvm::Function* currentFunction;
    
    // Stack for expression values
    std::stack<llvm::Value*> valueStack;
    
    // Helper methods
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* function, 
                                              const std::string& varName,
                                              llvm::Type* type);
    llvm::Value* getVariable(const std::string& name);
    void setVariable(const std::string& name, llvm::Value* value);
    void pushScope();
    void popScope();
    
    // Built-in functions
    void declareBuiltinFunctions();
    llvm::Function* declarePrintf();
    llvm::Function* declareScanf();
    llvm::Function* declareFgets();
    llvm::GlobalVariable* declareStdin();
    llvm::Function* declareSnprintf();
    llvm::Function* declareAtof();
    llvm::Function* declareAtoi();
    llvm::Function* declarePuts();
    
    // Math library functions
    llvm::Function* declareFabs();
    llvm::Function* declareRound();
    llvm::Function* declarePow();
    llvm::Function* declareSqrt();
    
    // Random number functions
    llvm::Function* declareRand();
    llvm::Function* declareSrand();
    
    // String library functions
    void declareStrlen();
    void declareMalloc();
    void declareStrcpy();
    void declareStrcat();
    void declareStrstr();
    
    // Type conversion helpers
    llvm::Value* convertToDouble(llvm::Value* value);
    llvm::Value* convertToInt(llvm::Value* value);
    llvm::Value* convertToBool(llvm::Value* value);
    llvm::Value* convertToString(llvm::Value* value);
    
    // String concatenation helper
    llvm::Value* createStringConcatenation(llvm::Value* left, llvm::Value* right);
    
public:
    CodeGenerator(const std::string& moduleName);
    ~CodeGenerator();
    
    // Generate LLVM IR from AST
    bool generate(Program* program);
    
    // Output LLVM IR
    void dumpIR();
    bool writeIRToFile(const std::string& filename);
    
    // Visitor methods
    void visit(Program* node) override;
    void visit(NumberLiteral* node) override;
    void visit(StringLiteral* node) override;
    void visit(BooleanLiteral* node) override;
    void visit(NullLiteral* node) override;
    void visit(Identifier* node) override;
    void visit(BinaryExpression* node) override;
    void visit(UnaryExpression* node) override;
    void visit(AssignmentExpression* node) override;
    void visit(CallExpression* node) override;
    void visit(ExpressionStatement* node) override;
    void visit(VariableDeclaration* node) override;
    void visit(BlockStatement* node) override;
    void visit(IfStatement* node) override;
    void visit(WhileStatement* node) override;
    void visit(ForStatement* node) override;
    void visit(ReturnStatement* node) override;
    void visit(FunctionDeclaration* node) override;
};

#endif // CODEGEN_H