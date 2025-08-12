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
    
    std::vector<std::map<std::string, llvm::AllocaInst*>> symbolTables;
    std::map<std::string, llvm::Function*> functions;
    
    llvm::Function* currentFunction;
    
    std::stack<llvm::Value*> valueStack;
    
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* function, 
                                              const std::string& varName,
                                              llvm::Type* type);
    llvm::Value* getVariable(const std::string& name);
    void setVariable(const std::string& name, llvm::Value* value);
    void pushScope();
    void popScope();
    
    // Built-ins
    void declareBuiltinFunctions();
    llvm::Function* declarePrintf();
    llvm::Function* declareScanf();
    llvm::Function* declareFgets();
    llvm::GlobalVariable* declareStdin();
    llvm::Function* declareSnprintf();
    llvm::Function* declareAtof();
    llvm::Function* declareAtoi();
    llvm::Function* declarePuts();
    
    // Math
    llvm::Function* declareFabs();
    llvm::Function* declareRound();
    llvm::Function* declareFloor();
    llvm::Function* declareCeil();
    llvm::Function* declareSin();
    llvm::Function* declareCos();
    llvm::Function* declareTan();
    llvm::Function* declarePow();
    llvm::Function* declareSqrt();
    llvm::Function* declareRand();
    llvm::Function* declareSrand();
    
    // String
    void declareStrlen();
    void declareMalloc();
    void declareStrcpy();
    void declareStrcat();
    void declareStrstr();
    
    // Conversion
    llvm::Value* convertToDouble(llvm::Value* value);
    llvm::Value* convertToInt(llvm::Value* value);
    llvm::Value* convertToBool(llvm::Value* value);
    llvm::Value* convertToString(llvm::Value* value);
    
    llvm::Value* getInt64(int64_t value);
    llvm::Value* createStringConcatenation(llvm::Value* left, llvm::Value* right);
    
    // Runtime boxing/unboxing
    llvm::Value* boxValue(llvm::Value* value);
    llvm::Value* unboxValue(llvm::Value* boxedValue, llvm::Type* expectedType);
    llvm::Value* unboxPointerToDouble(llvm::Value* ptrValue);
    llvm::Value* isStringPointer(llvm::Value* ptrValue, bool allowEmptyStrings);
    
    llvm::Value* createFormatString(const std::string& format);
    llvm::Value* getInt32(int32_t value);
    
public:
    CodeGenerator(const std::string& moduleName);
    ~CodeGenerator();
    
    bool generate(Program* program);
    
    void dumpIR();
    bool writeIRToFile(const std::string& filename);
    
    // Visitor
    void visit(Program* node) override;
    void visit(NumberLiteral* node) override;
    void visit(StringLiteral* node) override;
    void visit(BooleanLiteral* node) override;
    void visit(NullLiteral* node) override;
    void visit(Identifier* node) override;
    void visit(BinaryExpression* node) override;
    void visit(UnaryExpression* node) override;
    void visit(AssignmentExpression* node) override;
    void visit(IndexAssignmentExpression* node) override;
    void visit(CallExpression* node) override;
    void visit(ArrayLiteral* node) override;
    void visit(IndexExpression* node) override;
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