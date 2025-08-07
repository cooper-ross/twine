#include "../include/codegen.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <iostream>
#include <cstdlib>

CodeGenerator::CodeGenerator(const std::string& moduleName) {
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>(moduleName, *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
    currentFunction = nullptr;
    
    // Initialize with global scope
    pushScope();
    
    // Declare built-in functions
    declareBuiltinFunctions();
}

CodeGenerator::~CodeGenerator() = default;

void CodeGenerator::pushScope() {
    symbolTables.emplace_back();
}

void CodeGenerator::popScope() {
    if (!symbolTables.empty()) {
        symbolTables.pop_back();
    }
}

llvm::AllocaInst* CodeGenerator::createEntryBlockAlloca(llvm::Function* function, 
                                                         const std::string& varName,
                                                         llvm::Type* type) {
    llvm::IRBuilder<> tmpBuilder(&function->getEntryBlock(), 
                                  function->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, varName);
}

llvm::Value* CodeGenerator::getVariable(const std::string& name) {
    // Search from innermost to outermost scope
    for (auto it = symbolTables.rbegin(); it != symbolTables.rend(); ++it) {
        auto var = it->find(name);
        if (var != it->end()) {
            return builder->CreateLoad(var->second->getAllocatedType(), var->second, name);
        }
    }
    return nullptr;
}

void CodeGenerator::setVariable(const std::string& name, llvm::Value* value) {
    // Search for existing variable
    for (auto it = symbolTables.rbegin(); it != symbolTables.rend(); ++it) {
        auto var = it->find(name);
        if (var != it->end()) {
            // For dynamic typing, we need to handle type changes
            // Check if the value type matches the allocated type
            llvm::Type* allocatedType = var->second->getAllocatedType();
            if (value->getType() == allocatedType) {
                // Same type, direct store
            builder->CreateStore(value, var->second);
            } else {
                // Different type, need to create a new allocation
                llvm::AllocaInst* newAlloca = createEntryBlockAlloca(currentFunction, name + "_new", value->getType());
                builder->CreateStore(value, newAlloca);
                
                // Update the symbol table with the new allocation
                var->second = newAlloca;
            }
            return;
        }
    }
    
    // Create new variable in current scope
    if (currentFunction) {
        llvm::AllocaInst* alloca = createEntryBlockAlloca(currentFunction, name, value->getType());
        builder->CreateStore(value, alloca);
        symbolTables.back()[name] = alloca;
    }
}

void CodeGenerator::declareBuiltinFunctions() {
    // Declare printf for print implementation
    declarePrintf();
    
    // Declare scanf for input implementation
    declareScanf();
    
    // Declare fgets for better input handling
    declareFgets();
    
    // Declare stdin
    declareStdin();
    
    // Declare conversion helper functions
    declareSnprintf();
    declareAtof();
    declareAtoi();
    
    // Declare math helper functions
    declareFabs();
    declareRound();
    declarePow();
    declareSqrt();
    
    // Declare string helper functions
    declareStrlen();
    declareMalloc();
    declareStrcpy();
    declareStrcat();
    
    // Create print function that wraps printf
    std::vector<llvm::Type*> printParams = {
        llvm::PointerType::getUnqual(*context)  // pointer for string (opaque pointer)
    };
    
    llvm::FunctionType* printType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(*context),
        printParams,
        true  // variadic
    );
    
    llvm::Function* printFunc = llvm::Function::Create(
        printType,
        llvm::Function::ExternalLinkage,
        "print",
        module.get()
    );
    
    functions["print"] = printFunc;
    
    // Create input function that wraps scanf
    std::vector<llvm::Type*> inputParams = {};
    
    llvm::FunctionType* inputType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),  // returns string pointer
        inputParams,
        false  // not variadic
    );
    
    llvm::Function* inputFunc = llvm::Function::Create(
        inputType,
        llvm::Function::ExternalLinkage,
        "input",
        module.get()
    );
    
    functions["input"] = inputFunc;
    
    // Create str function (convert number to string)
    std::vector<llvm::Type*> strParams = {
        llvm::Type::getDoubleTy(*context)  // takes a double
    };
    
    llvm::FunctionType* strType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),  // returns string pointer
        strParams,
        false  // not variadic
    );
    
    llvm::Function* strFunc = llvm::Function::Create(
        strType,
        llvm::Function::ExternalLinkage,
        "str",
        module.get()
    );
    
    functions["str"] = strFunc;
    
    // Create num function (convert string to number)
    std::vector<llvm::Type*> numParams = {
        llvm::PointerType::getUnqual(*context)  // takes a string
    };
    
    llvm::FunctionType* numType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double
        numParams,
        false  // not variadic
    );
    
    llvm::Function* numFunc = llvm::Function::Create(
        numType,
        llvm::Function::ExternalLinkage,
        "num",
        module.get()
    );
    
    functions["num"] = numFunc;
    
    // Create int function (convert string to integer)
    std::vector<llvm::Type*> intParams = {
        llvm::PointerType::getUnqual(*context)  // takes a string
    };
    
    llvm::FunctionType* intType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double (for consistency)
        intParams,
        false  // not variadic
    );
    
    llvm::Function* intFunc = llvm::Function::Create(
        intType,
        llvm::Function::ExternalLinkage,
        "int",
        module.get()
    );
    
    functions["int"] = intFunc;
    
    // Create abs function
    std::vector<llvm::Type*> absParams = {
        llvm::Type::getDoubleTy(*context)  // takes a double
    };
    
    llvm::FunctionType* absType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double
        absParams,
        false  // not variadic
    );
    
    llvm::Function* absFunc = llvm::Function::Create(
        absType,
        llvm::Function::ExternalLinkage,
        "abs",
        module.get()
    );
    
    functions["abs"] = absFunc;
    
    // Create round function
    std::vector<llvm::Type*> roundParams = {
        llvm::Type::getDoubleTy(*context)  // takes a double
    };
    
    llvm::FunctionType* roundType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double
        roundParams,
        true  // variadic - to support optional precision parameter
    );
    
    llvm::Function* roundFunc = llvm::Function::Create(
        roundType,
        llvm::Function::ExternalLinkage,
        "round",
        module.get()
    );
    
    functions["round"] = roundFunc;
    
    // Create min function (variadic)
    std::vector<llvm::Type*> minParams = {
        llvm::Type::getDoubleTy(*context),  // takes at least one double
        llvm::Type::getDoubleTy(*context)   // takes at least two arguments
    };
    
    llvm::FunctionType* minType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double
        minParams,
        true  // variadic - for more than two arguments
    );
    
    llvm::Function* minFunc = llvm::Function::Create(
        minType,
        llvm::Function::ExternalLinkage,
        "min",
        module.get()
    );
    
    functions["min"] = minFunc;
    
    // Create max function (variadic)
    std::vector<llvm::Type*> maxParams = {
        llvm::Type::getDoubleTy(*context),  // takes at least one double
        llvm::Type::getDoubleTy(*context)   // takes at least two arguments
    };
    
    llvm::FunctionType* maxType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double
        maxParams,
        true  // variadic - for more than two arguments
    );
    
    llvm::Function* maxFunc = llvm::Function::Create(
        maxType,
        llvm::Function::ExternalLinkage,
        "max",
        module.get()
    );
    
    functions["max"] = maxFunc;
    
    // Create pow function
    std::vector<llvm::Type*> powParams = {
        llvm::Type::getDoubleTy(*context),  // base
        llvm::Type::getDoubleTy(*context)   // exponent
    };
    
    llvm::FunctionType* powType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double
        powParams,
        false  // not variadic
    );
    
    llvm::Function* powFunc = llvm::Function::Create(
        powType,
        llvm::Function::ExternalLinkage,
        "pow",
        module.get()
    );
    
    functions["pow"] = powFunc;
    
    // Create sqrt function
    std::vector<llvm::Type*> sqrtParams = {
        llvm::Type::getDoubleTy(*context)  // takes a double
    };
    
    llvm::FunctionType* sqrtType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double
        sqrtParams,
        false  // not variadic
    );
    
    llvm::Function* sqrtFunc = llvm::Function::Create(
        sqrtType,
        llvm::Function::ExternalLinkage,
        "sqrt",
        module.get()
    );
    
    functions["sqrt"] = sqrtFunc;
}

llvm::Function* CodeGenerator::declarePrintf() {
    std::vector<llvm::Type*> printfParams = {
        llvm::PointerType::getUnqual(*context)  // pointer for format string (opaque pointer)
    };
    
    llvm::FunctionType* printfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),
        printfParams,
        true  // variadic
    );
    
    llvm::Function* printfFunc = llvm::Function::Create(
        printfType,
        llvm::Function::ExternalLinkage,
        "printf",
        module.get()
    );
    
    functions["printf"] = printfFunc;
    return printfFunc;
}

llvm::Function* CodeGenerator::declareScanf() {
    std::vector<llvm::Type*> scanfParams = {
        llvm::PointerType::getUnqual(*context)  // pointer for format string (opaque pointer)
    };
    
    llvm::FunctionType* scanfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),
        scanfParams,
        true  // variadic
    );
    
    llvm::Function* scanfFunc = llvm::Function::Create(
        scanfType,
        llvm::Function::ExternalLinkage,
        "scanf",
        module.get()
    );
    
    functions["scanf"] = scanfFunc;
    return scanfFunc;
}

llvm::Function* CodeGenerator::declareFgets() {
    // char *fgets(char *str, int n, FILE *stream)
    std::vector<llvm::Type*> fgetsParams = {
        llvm::PointerType::getUnqual(*context),  // char* str
        llvm::Type::getInt32Ty(*context),        // int n
        llvm::PointerType::getUnqual(*context)   // FILE* stream
    };
    
    llvm::FunctionType* fgetsType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),  // returns char*
        fgetsParams,
        false  // not variadic
    );
    
    llvm::Function* fgetsFunc = llvm::Function::Create(
        fgetsType,
        llvm::Function::ExternalLinkage,
        "fgets",
        module.get()
    );
    
    functions["fgets"] = fgetsFunc;
    return fgetsFunc;
}

llvm::GlobalVariable* CodeGenerator::declareStdin() {
#ifdef _WIN32
    // On Windows with MinGW, stdin is accessed via __acrt_iob_func
    // We'll declare this function and call it with index 0 to get stdin
    llvm::FunctionType* iobFuncType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),  // returns FILE*
        {},  // no parameters for our purposes
        false
    );
    
    // We'll create a helper function that returns stdin
    llvm::Function* getStdinFunc = llvm::Function::Create(
        iobFuncType,
        llvm::Function::ExternalLinkage,
        "get_stdin_ptr",
        module.get()
    );
    
    // Create the function body
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", getStdinFunc);
    llvm::IRBuilder<> tmpBuilder(entry);
    
    // Declare __acrt_iob_func
    llvm::FunctionType* acrtIobFuncType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),  // returns FILE*
        {llvm::Type::getInt32Ty(*context)},      // takes int index
        false
    );
    
    llvm::Function* acrtIobFunc = module->getFunction("__acrt_iob_func");
    if (!acrtIobFunc) {
        acrtIobFunc = llvm::Function::Create(
            acrtIobFuncType,
            llvm::Function::ExternalLinkage,
            "__acrt_iob_func",
            module.get()
        );
    }
    
    // Call __acrt_iob_func(0) to get stdin
    llvm::Value* zero = llvm::ConstantInt::get(*context, llvm::APInt(32, 0));
    llvm::Value* stdinPtr = tmpBuilder.CreateCall(acrtIobFunc, {zero});
    tmpBuilder.CreateRet(stdinPtr);
    
    // Create a global that will hold the stdin pointer
    llvm::GlobalVariable* stdinVar = new llvm::GlobalVariable(
        *module,
        llvm::PointerType::getUnqual(*context),
        false,
        llvm::GlobalValue::InternalLinkage,
        llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context)),
        "stdin_ptr"
    );
    
    return stdinVar;
#else
    // On Unix-like systems, stdin is a simple external global
    llvm::Type* fileType = llvm::PointerType::getUnqual(*context);  // FILE* type
    
    llvm::GlobalVariable* stdinVar = new llvm::GlobalVariable(
        *module,
        fileType,
        false,  // not constant
        llvm::GlobalValue::ExternalLinkage,
        nullptr,  // no initializer
        "stdin"
    );
    
    return stdinVar;
#endif
}

llvm::Function* CodeGenerator::declareSnprintf() {
    // int snprintf(char *str, size_t size, const char *format, ...)
    std::vector<llvm::Type*> snprintfParams = {
        llvm::PointerType::getUnqual(*context),  // char* str
        llvm::Type::getInt64Ty(*context),        // size_t size
        llvm::PointerType::getUnqual(*context)   // const char* format
    };
    
    llvm::FunctionType* snprintfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),  // returns int
        snprintfParams,
        true  // variadic
    );
    
    llvm::Function* snprintfFunc = llvm::Function::Create(
        snprintfType,
        llvm::Function::ExternalLinkage,
        "snprintf",
        module.get()
    );
    
    functions["snprintf"] = snprintfFunc;
    return snprintfFunc;
}

llvm::Function* CodeGenerator::declareAtof() {
    // double atof(const char *str)
    std::vector<llvm::Type*> atofParams = {
        llvm::PointerType::getUnqual(*context)  // const char* str
    };
    
    llvm::FunctionType* atofType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double
        atofParams,
        false  // not variadic
    );
    
    llvm::Function* atofFunc = llvm::Function::Create(
        atofType,
        llvm::Function::ExternalLinkage,
        "atof",
        module.get()
    );
    
    functions["atof"] = atofFunc;
    return atofFunc;
}

llvm::Function* CodeGenerator::declareAtoi() {
    // int atoi(const char *str)
    std::vector<llvm::Type*> atoiParams = {
        llvm::PointerType::getUnqual(*context)  // const char* str
    };
    
    llvm::FunctionType* atoiType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),  // returns int
        atoiParams,
        false  // not variadic
    );
    
    llvm::Function* atoiFunc = llvm::Function::Create(
        atoiType,
        llvm::Function::ExternalLinkage,
        "atoi",
        module.get()
    );
    
    functions["atoi"] = atoiFunc;
    return atoiFunc;
}

llvm::Function* CodeGenerator::declareFabs() {
    // double fabs(double x)
    std::vector<llvm::Type*> fabsParams = {
        llvm::Type::getDoubleTy(*context)  // double x
    };
    
    llvm::FunctionType* fabsType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double
        fabsParams,
        false  // not variadic
    );
    
    llvm::Function* fabsFunc = llvm::Function::Create(
        fabsType,
        llvm::Function::ExternalLinkage,
        "fabs",
        module.get()
    );
    
    functions["fabs"] = fabsFunc;
    return fabsFunc;
}

llvm::Function* CodeGenerator::declareRound() {
    // double round(double x)
    std::vector<llvm::Type*> roundParams = {
        llvm::Type::getDoubleTy(*context)  // double x
    };
    
    llvm::FunctionType* roundType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double
        roundParams,
        false  // not variadic
    );
    
    llvm::Function* roundFunc = llvm::Function::Create(
        roundType,
        llvm::Function::ExternalLinkage,
        "round",
        module.get()
    );
    
    functions["mathRound"] = roundFunc;  // Store as mathRound to avoid name clash with our built-in round
    return roundFunc;
}

llvm::Function* CodeGenerator::declarePow() {
    // double pow(double base, double exponent)
    std::vector<llvm::Type*> powParams = {
        llvm::Type::getDoubleTy(*context),  // double base
        llvm::Type::getDoubleTy(*context)   // double exponent
    };
    
    llvm::FunctionType* powType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double
        powParams,
        false  // not variadic
    );
    
    llvm::Function* powFunc = llvm::Function::Create(
        powType,
        llvm::Function::ExternalLinkage,
        "pow",
        module.get()
    );
    
    functions["mathPow"] = powFunc;  // Store as mathPow to avoid name clash with our built-in pow
    return powFunc;
}

llvm::Function* CodeGenerator::declareSqrt() {
    // double sqrt(double x)
    std::vector<llvm::Type*> sqrtParams = {
        llvm::Type::getDoubleTy(*context)  // double x
    };
    
    llvm::FunctionType* sqrtType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),  // returns double
        sqrtParams,
        false  // not variadic
    );
    
    llvm::Function* sqrtFunc = llvm::Function::Create(
        sqrtType,
        llvm::Function::ExternalLinkage,
        "sqrt",
        module.get()
    );
    
    functions["mathSqrt"] = sqrtFunc;  // Store as mathSqrt to avoid name clash with our built-in sqrt
    return sqrtFunc;
}

llvm::Function* CodeGenerator::declarePuts() {
    std::vector<llvm::Type*> putsParams = {
        llvm::PointerType::getUnqual(*context)  // pointer for string (opaque pointer)
    };
    
    llvm::FunctionType* putsType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),
        putsParams,
        false
    );
    
    llvm::Function* putsFunc = llvm::Function::Create(
        putsType,
        llvm::Function::ExternalLinkage,
        "puts",
        module.get()
    );
    
    functions["puts"] = putsFunc;
    return putsFunc;
}

llvm::Value* CodeGenerator::convertToDouble(llvm::Value* value) {
    if (value->getType()->isDoubleTy()) {
        return value;
    } else if (value->getType()->isIntegerTy()) {
        return builder->CreateSIToFP(value, llvm::Type::getDoubleTy(*context), "cast");
    }
    return value;
}

llvm::Value* CodeGenerator::convertToInt(llvm::Value* value) {
    if (value->getType()->isIntegerTy()) {
        return value;
    } else if (value->getType()->isDoubleTy()) {
        return builder->CreateFPToSI(value, llvm::Type::getInt32Ty(*context), "cast");
    }
    return value;
}

llvm::Value* CodeGenerator::convertToBool(llvm::Value* value) {
    if (value->getType()->isIntegerTy(1)) {
        return value;
    } else if (value->getType()->isIntegerTy()) {
        return builder->CreateICmpNE(value, 
            llvm::ConstantInt::get(value->getType(), 0), "tobool");
    } else if (value->getType()->isDoubleTy()) {
        return builder->CreateFCmpONE(value, 
            llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), "tobool");
    }
    return value;
}

bool CodeGenerator::generate(Program* program) {
    try {
        // Create main function
        llvm::FunctionType* mainType = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*context),
            false
        );
        
        llvm::Function* mainFunc = llvm::Function::Create(
            mainType,
            llvm::Function::ExternalLinkage,
            "main",
            module.get()
        );
        
        llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", mainFunc);
        builder->SetInsertPoint(entry);
        
        currentFunction = mainFunc;
        
        // Visit the program
        program->accept(this);
        
        // Return 0 from main
        builder->CreateRet(llvm::ConstantInt::get(*context, llvm::APInt(32, 0)));
        
        // Verify the module
        std::string error;
        llvm::raw_string_ostream errorStream(error);
        if (llvm::verifyModule(*module, &errorStream)) {
            std::cerr << "Module verification failed: " << error << std::endl;
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Code generation error: " << e.what() << std::endl;
        return false;
    }
}

void CodeGenerator::dumpIR() {
    module->print(llvm::outs(), nullptr);
}

bool CodeGenerator::writeIRToFile(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream out(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        std::cerr << "Error opening file: " << EC.message() << std::endl;
        return false;
    }
    
    module->print(out, nullptr);
    return true;
}

// Visitor implementations

void CodeGenerator::visit(Program* node) {
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
}

void CodeGenerator::visit(NumberLiteral* node) {
    valueStack.push(llvm::ConstantFP::get(*context, llvm::APFloat(node->value)));
}

void CodeGenerator::visit(StringLiteral* node) {
    // Create a global string constant
    llvm::Constant* strConstant = llvm::ConstantDataArray::getString(*context, node->value);
    
    llvm::GlobalVariable* globalStr = new llvm::GlobalVariable(
        *module,
        strConstant->getType(),
        true,  // isConstant
        llvm::GlobalValue::PrivateLinkage,
        strConstant,
        ".str"
    );
    
    // Get pointer to the string
    std::vector<llvm::Value*> indices = {
        llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
        llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
    };
    
    llvm::Value* strPtr = builder->CreateInBoundsGEP(
        globalStr->getValueType(),
        globalStr,
        indices,
        "str"
    );
    
    valueStack.push(strPtr);
}

void CodeGenerator::visit(BooleanLiteral* node) {
    valueStack.push(llvm::ConstantInt::get(*context, llvm::APInt(1, node->value ? 1 : 0)));
}

void CodeGenerator::visit(NullLiteral* node) {
    valueStack.push(llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context)));
}

void CodeGenerator::visit(Identifier* node) {
    llvm::Value* value = getVariable(node->name);
    if (!value) {
        throw std::runtime_error("Undefined variable: " + node->name);
    }
    valueStack.push(value);
}

void CodeGenerator::visit(BinaryExpression* node) {
    node->left->accept(this);
    llvm::Value* left = valueStack.top();
    valueStack.pop();
    
    node->right->accept(this);
    llvm::Value* right = valueStack.top();
    valueStack.pop();
    
    llvm::Value* result = nullptr;
    
    if (node->op == "+") {
        // Check if either operand is a string (pointer type)
        if (left->getType()->isPointerTy() || right->getType()->isPointerTy()) {
            // Handle string concatenation
            result = createStringConcatenation(left, right);
        } else if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
            left = convertToDouble(left);
            right = convertToDouble(right);
            result = builder->CreateFAdd(left, right, "add");
        } else {
            result = builder->CreateAdd(left, right, "add");
        }
    } else if (node->op == "-") {
        if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
            left = convertToDouble(left);
            right = convertToDouble(right);
            result = builder->CreateFSub(left, right, "sub");
        } else {
            result = builder->CreateSub(left, right, "sub");
        }
    } else if (node->op == "*") {
        if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
            left = convertToDouble(left);
            right = convertToDouble(right);
            result = builder->CreateFMul(left, right, "mul");
        } else {
            result = builder->CreateMul(left, right, "mul");
        }
    } else if (node->op == "/") {
        left = convertToDouble(left);
        right = convertToDouble(right);
        result = builder->CreateFDiv(left, right, "div");
    } else if (node->op == "%") {
        if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
            left = convertToDouble(left);
            right = convertToDouble(right);
            result = builder->CreateFRem(left, right, "mod");
        } else {
            result = builder->CreateSRem(left, right, "mod");
        }
    } else if (node->op == "==") {
        if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
            left = convertToDouble(left);
            right = convertToDouble(right);
            result = builder->CreateFCmpOEQ(left, right, "eq");
        } else {
            result = builder->CreateICmpEQ(left, right, "eq");
        }
    } else if (node->op == "!=") {
        if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
            left = convertToDouble(left);
            right = convertToDouble(right);
            result = builder->CreateFCmpONE(left, right, "ne");
        } else {
            result = builder->CreateICmpNE(left, right, "ne");
        }
    } else if (node->op == "<") {
        if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
            left = convertToDouble(left);
            right = convertToDouble(right);
            result = builder->CreateFCmpOLT(left, right, "lt");
        } else {
            result = builder->CreateICmpSLT(left, right, "lt");
        }
    } else if (node->op == ">") {
        if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
            left = convertToDouble(left);
            right = convertToDouble(right);
            result = builder->CreateFCmpOGT(left, right, "gt");
        } else {
            result = builder->CreateICmpSGT(left, right, "gt");
        }
    } else if (node->op == "<=") {
        if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
            left = convertToDouble(left);
            right = convertToDouble(right);
            result = builder->CreateFCmpOLE(left, right, "le");
        } else {
            result = builder->CreateICmpSLE(left, right, "le");
        }
    } else if (node->op == ">=") {
        if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
            left = convertToDouble(left);
            right = convertToDouble(right);
            result = builder->CreateFCmpOGE(left, right, "ge");
        } else {
            result = builder->CreateICmpSGE(left, right, "ge");
        }
    } else if (node->op == "&&") {
        left = convertToBool(left);
        right = convertToBool(right);
        result = builder->CreateLogicalAnd(left, right, "and");
    } else if (node->op == "||") {
        left = convertToBool(left);
        right = convertToBool(right);
        result = builder->CreateLogicalOr(left, right, "or");
    }
    
    if (result) {
        valueStack.push(result);
    } else {
        throw std::runtime_error("Unknown binary operator: " + node->op);
    }
}

void CodeGenerator::visit(UnaryExpression* node) {
    node->operand->accept(this);
    llvm::Value* operand = valueStack.top();
    valueStack.pop();
    
    llvm::Value* result = nullptr;
    
    if (node->op == "-") {
        if (operand->getType()->isDoubleTy()) {
            result = builder->CreateFNeg(operand, "neg");
        } else {
            result = builder->CreateNeg(operand, "neg");
        }
    } else if (node->op == "!") {
        operand = convertToBool(operand);
        result = builder->CreateNot(operand, "not");
    }
    
    if (result) {
        valueStack.push(result);
    } else {
        throw std::runtime_error("Unknown unary operator: " + node->op);
    }
}

void CodeGenerator::visit(AssignmentExpression* node) {
    node->value->accept(this);
    llvm::Value* value = valueStack.top();
    valueStack.pop();
    
    setVariable(node->name, value);
    valueStack.push(value);
}

void CodeGenerator::visit(CallExpression* node) {
    if (node->name == "input") {
        // Handle input specially - reads a line from stdin
        if (!node->arguments.empty()) {
            std::cerr << "Warning: input() function takes no arguments, ignoring provided arguments" << std::endl;
        }
        
        // Allocate buffer for input (1024 characters should be enough)
        llvm::Type* charType = llvm::Type::getInt8Ty(*context);
        llvm::Type* arrayType = llvm::ArrayType::get(charType, 1024);
        llvm::AllocaInst* buffer = builder->CreateAlloca(arrayType, nullptr, "input_buffer");
        
        // Get pointer to first element of array
        std::vector<llvm::Value*> indices = {
            llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
        };
        llvm::Value* bufferPtr = builder->CreateInBoundsGEP(
            arrayType,
            buffer,
            indices
        );
        
        // Get stdin pointer
        llvm::Value* stdinPtr;
#ifdef _WIN32
        // On Windows, call our helper function to get stdin
        llvm::Function* getStdinFunc = module->getFunction("get_stdin_ptr");
        if (!getStdinFunc) {
            // Recreate the helper if it doesn't exist
            declareStdin();
            getStdinFunc = module->getFunction("get_stdin_ptr");
        }
        stdinPtr = builder->CreateCall(getStdinFunc, {}, "stdin_ptr");
#else
        // On Unix, load from the stdin global
        llvm::GlobalVariable* stdinVar = module->getGlobalVariable("stdin");
        if (!stdinVar) {
            stdinVar = declareStdin();
        }
        stdinPtr = builder->CreateLoad(llvm::PointerType::getUnqual(*context), stdinVar, "stdin_load");
#endif
        
        // Call fgets to read a full line (up to 1023 chars + null terminator)
        llvm::Value* bufferSize = llvm::ConstantInt::get(*context, llvm::APInt(32, 1024));
        llvm::Value* result = builder->CreateCall(functions["fgets"], {bufferPtr, bufferSize, stdinPtr});
        
        // Remove trailing newline if present
        // First, find the length of the string
        llvm::Function* strlenFunc = module->getFunction("strlen");
        if (!strlenFunc) {
            // Declare strlen if not already declared
            llvm::FunctionType* strlenType = llvm::FunctionType::get(
                llvm::Type::getInt64Ty(*context),
                {llvm::PointerType::getUnqual(*context)},
                false
            );
            strlenFunc = llvm::Function::Create(
                strlenType,
                llvm::Function::ExternalLinkage,
                "strlen",
                module.get()
            );
        }
        
        llvm::Value* length = builder->CreateCall(strlenFunc, {bufferPtr});
        
        // Check if last character is newline and replace with null terminator
        llvm::Value* one = llvm::ConstantInt::get(*context, llvm::APInt(64, 1));
        llvm::Value* lastCharIndex = builder->CreateSub(length, one);
        llvm::Value* lastCharPtr = builder->CreateInBoundsGEP(charType, bufferPtr, lastCharIndex);
        llvm::Value* lastChar = builder->CreateLoad(charType, lastCharPtr);
        llvm::Value* newline = llvm::ConstantInt::get(charType, 10); // '\n'
        llvm::Value* isNewline = builder->CreateICmpEQ(lastChar, newline);
        
        // Create basic blocks for conditional
        llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(*context, "remove_newline", currentFunction);
        llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context, "merge", currentFunction);
        
        builder->CreateCondBr(isNewline, thenBlock, mergeBlock);
        
        // Then block: replace newline with null terminator
        builder->SetInsertPoint(thenBlock);
        llvm::Value* nullChar = llvm::ConstantInt::get(charType, 0);
        builder->CreateStore(nullChar, lastCharPtr);
        builder->CreateBr(mergeBlock);
        
        // Merge block
        builder->SetInsertPoint(mergeBlock);
        
        // Return the buffer pointer as the result
        valueStack.push(bufferPtr);
        return;
    } else if (node->name == "str") {
        // Handle str specially - converts number to string
        if (node->arguments.size() != 1) {
            std::cerr << "Error: str() expects exactly 1 argument" << std::endl;
            return;
        }
        
        // Evaluate the argument
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        // Convert to double if needed
        if (!value->getType()->isDoubleTy()) {
            value = convertToDouble(value);
        }
        
        // Allocate buffer for string result (32 characters should be enough for most numbers)
        llvm::Type* charType = llvm::Type::getInt8Ty(*context);
        llvm::Type* arrayType = llvm::ArrayType::get(charType, 32);
        llvm::AllocaInst* buffer = builder->CreateAlloca(arrayType, nullptr, "str_buffer");
        
        // Get pointer to first element of array
        std::vector<llvm::Value*> indices = {
            llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
        };
        llvm::Value* bufferPtr = builder->CreateInBoundsGEP(
            arrayType,
            buffer,
            indices
        );
        
        // Create format string for snprintf ("%g" for general format)
        llvm::GlobalVariable* format = builder->CreateGlobalString("%g");
        std::vector<llvm::Value*> formatIndices = {
            llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
        };
        llvm::Value* formatPtr = builder->CreateInBoundsGEP(
            format->getValueType(),
            format,
            formatIndices
        );
        
        // Call snprintf to convert number to string
        llvm::Value* bufferSize = llvm::ConstantInt::get(*context, llvm::APInt(64, 32));
        builder->CreateCall(functions["snprintf"], {bufferPtr, bufferSize, formatPtr, value});
        
        // Return the buffer pointer as the result
        valueStack.push(bufferPtr);
        return;
    } else if (node->name == "num") {
        // Handle num specially - converts string to number using atof
        if (node->arguments.size() != 1) {
            std::cerr << "Error: num() expects exactly 1 argument" << std::endl;
            return;
        }
        
        // Evaluate the argument
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        if (!value->getType()->isPointerTy()) {
            std::cerr << "Error: num() expects a string argument" << std::endl;
            return;
        }
        
        // Call atof to convert string to double
        llvm::Value* result = builder->CreateCall(functions["atof"], {value});
        
        // Return the result
        valueStack.push(result);
        return;
    } else if (node->name == "int") {
        // Handle int specially - converts string to integer using atoi, then cast to double
        if (node->arguments.size() != 1) {
            std::cerr << "Error: int() expects exactly 1 argument" << std::endl;
            return;
        }
        
        // Evaluate the argument
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        if (!value->getType()->isPointerTy()) {
            std::cerr << "Error: int() expects a string argument" << std::endl;
            return;
        }
        
        // Call atoi to convert string to int
        llvm::Value* intResult = builder->CreateCall(functions["atoi"], {value});
        
        // Convert int to double for consistency with our type system
        llvm::Value* result = builder->CreateSIToFP(intResult, llvm::Type::getDoubleTy(*context));
        
        // Return the result
        valueStack.push(result);
        return;
    } else if (node->name == "abs") {
        // Handle abs specially - absolute value
        if (node->arguments.size() != 1) {
            std::cerr << "Error: abs() expects exactly 1 argument" << std::endl;
            return;
        }
        
        // Evaluate the argument
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        // Convert to double if needed
        if (!value->getType()->isDoubleTy()) {
            value = convertToDouble(value);
        }
        
        // Call fabs (C standard library) for absolute value
        llvm::Value* result = builder->CreateCall(functions["fabs"], {value});
        
        // Return the result
        valueStack.push(result);
        return;
    } else if (node->name == "round") {
        // Handle round specially - round to nearest integer
        if (node->arguments.size() < 1 || node->arguments.size() > 2) {
            std::cerr << "Error: round() expects 1 or 2 arguments" << std::endl;
            return;
        }
        
        // Evaluate the first argument (the number to round)
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        // Convert to double if needed
        if (!value->getType()->isDoubleTy()) {
            value = convertToDouble(value);
        }
        
        if (node->arguments.size() == 1) {
            // Simple case: round to integer
            llvm::Value* result = builder->CreateCall(functions["mathRound"], {value});
            valueStack.push(result);
        } else {
            // Round to specified decimal places
            // Evaluate the second argument (decimal places)
            node->arguments[1]->accept(this);
            llvm::Value* decimalPlaces = valueStack.top();
            valueStack.pop();
            
            // Convert to int if needed
            if (!decimalPlaces->getType()->isDoubleTy()) {
                decimalPlaces = convertToDouble(decimalPlaces);
            }
            
            // We'll use the formula: round(x * 10^d) / 10^d
            // First, calculate 10^d (scale factor)
            llvm::Value* ten = llvm::ConstantFP::get(*context, llvm::APFloat(10.0));
            llvm::Value* scaleFactor = builder->CreateCall(functions["mathPow"], {ten, decimalPlaces});
            
            // Multiply the value by the scale factor
            llvm::Value* scaled = builder->CreateFMul(value, scaleFactor);
            
            // Round the scaled value
            llvm::Value* roundedScaled = builder->CreateCall(functions["mathRound"], {scaled});
            
            // Divide by the scale factor to get the final result
            llvm::Value* result = builder->CreateFDiv(roundedScaled, scaleFactor);
            
            valueStack.push(result);
        }
        return;
    } else if (node->name == "min") {
        // Handle min specially - minimum of values
        if (node->arguments.size() < 2) {
            std::cerr << "Error: min() expects at least 2 arguments" << std::endl;
            return;
        }
        
        // Evaluate first argument
        node->arguments[0]->accept(this);
        llvm::Value* minValue = valueStack.top();
        valueStack.pop();
        
        // Convert to double if needed
        if (!minValue->getType()->isDoubleTy()) {
            minValue = convertToDouble(minValue);
        }
        
        // Compare with remaining arguments to find the minimum
        for (size_t i = 1; i < node->arguments.size(); i++) {
            node->arguments[i]->accept(this);
            llvm::Value* currentValue = valueStack.top();
            valueStack.pop();
            
            // Convert to double if needed
            if (!currentValue->getType()->isDoubleTy()) {
                currentValue = convertToDouble(currentValue);
            }
            
            // Compare current value with minValue and update if needed
            llvm::Value* cmp = builder->CreateFCmpOLT(currentValue, minValue);
            minValue = builder->CreateSelect(cmp, currentValue, minValue);
        }
        
        // Return the minimum value
        valueStack.push(minValue);
        return;
    } else if (node->name == "max") {
        // Handle max specially - maximum of values
        if (node->arguments.size() < 2) {
            std::cerr << "Error: max() expects at least 2 arguments" << std::endl;
            return;
        }
        
        // Evaluate first argument
        node->arguments[0]->accept(this);
        llvm::Value* maxValue = valueStack.top();
        valueStack.pop();
        
        // Convert to double if needed
        if (!maxValue->getType()->isDoubleTy()) {
            maxValue = convertToDouble(maxValue);
        }
        
        // Compare with remaining arguments to find the maximum
        for (size_t i = 1; i < node->arguments.size(); i++) {
            node->arguments[i]->accept(this);
            llvm::Value* currentValue = valueStack.top();
            valueStack.pop();
            
            // Convert to double if needed
            if (!currentValue->getType()->isDoubleTy()) {
                currentValue = convertToDouble(currentValue);
            }
            
            // Compare current value with maxValue and update if needed
            llvm::Value* cmp = builder->CreateFCmpOGT(currentValue, maxValue);
            maxValue = builder->CreateSelect(cmp, currentValue, maxValue);
        }
        
        // Return the maximum value
        valueStack.push(maxValue);
        return;
    } else if (node->name == "pow") {
        // Handle pow specially - raise to power
        if (node->arguments.size() != 2) {
            std::cerr << "Error: pow() expects exactly 2 arguments" << std::endl;
            return;
        }
        
        // Evaluate first argument (base)
        node->arguments[0]->accept(this);
        llvm::Value* base = valueStack.top();
        valueStack.pop();
        
        // Convert to double if needed
        if (!base->getType()->isDoubleTy()) {
            base = convertToDouble(base);
        }
        
        // Evaluate second argument (exponent)
        node->arguments[1]->accept(this);
        llvm::Value* exponent = valueStack.top();
        valueStack.pop();
        
        // Convert to double if needed
        if (!exponent->getType()->isDoubleTy()) {
            exponent = convertToDouble(exponent);
        }
        
        // Call pow (C standard library) for power operation
        llvm::Value* result = builder->CreateCall(functions["mathPow"], {base, exponent});
        
        // Return the result
        valueStack.push(result);
        return;
    } else if (node->name == "sqrt") {
        // Handle sqrt specially - square root
        if (node->arguments.size() != 1) {
            std::cerr << "Error: sqrt() expects exactly 1 argument" << std::endl;
            return;
        }
        
        // Evaluate the argument
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        // Convert to double if needed
        if (!value->getType()->isDoubleTy()) {
            value = convertToDouble(value);
        }
        
        // Call sqrt (C standard library) for square root
        llvm::Value* result = builder->CreateCall(functions["mathSqrt"], {value});
        
        // Return the result
        valueStack.push(result);
        return;
    } else if (node->name == "print") {
        // Handle print specially
        if (node->arguments.empty()) {
            // Print newline only
            llvm::GlobalVariable* newline = builder->CreateGlobalString("\n");
            std::vector<llvm::Value*> indices = {
                llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
                llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
            };
            llvm::Value* newlinePtr = builder->CreateInBoundsGEP(
                newline->getValueType(),
                newline,
                indices
            );
            builder->CreateCall(functions["printf"], {newlinePtr});
        } else {
            // Evaluate arguments
            for (auto& arg : node->arguments) {
                arg->accept(this);
                llvm::Value* value = valueStack.top();
                valueStack.pop();
                
                if (value->getType()->isPointerTy()) {
                    // String - print with %s
                    llvm::GlobalVariable* format = builder->CreateGlobalString("%s\n");
                    std::vector<llvm::Value*> indices = {
                        llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
                        llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
                    };
                    llvm::Value* formatPtr = builder->CreateInBoundsGEP(
                        format->getValueType(),
                        format,
                        indices
                    );
                    builder->CreateCall(functions["printf"], {formatPtr, value});
                } else if (value->getType()->isDoubleTy()) {
                    // Double - print with %f
                    llvm::GlobalVariable* format = builder->CreateGlobalString("%f\n");
                    std::vector<llvm::Value*> indices = {
                        llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
                        llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
                    };
                    llvm::Value* formatPtr = builder->CreateInBoundsGEP(
                        format->getValueType(),
                        format,
                        indices
                    );
                    builder->CreateCall(functions["printf"], {formatPtr, value});
                } else if (value->getType()->isIntegerTy()) {
                    // Integer - print with %d
                    llvm::GlobalVariable* format = builder->CreateGlobalString("%d\n");
                    std::vector<llvm::Value*> indices = {
                        llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
                        llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
                    };
                    llvm::Value* formatPtr = builder->CreateInBoundsGEP(
                        format->getValueType(),
                        format,
                        indices
                    );
                    builder->CreateCall(functions["printf"], {formatPtr, value});
                }
            }
        }
        valueStack.push(llvm::ConstantInt::get(*context, llvm::APInt(32, 0)));
    } else {
        // Regular function call
        auto it = functions.find(node->name);
        if (it == functions.end()) {
            throw std::runtime_error("Undefined function: " + node->name);
        }
        
        llvm::Function* func = it->second;
        std::vector<llvm::Value*> args;
        
        for (auto& arg : node->arguments) {
            arg->accept(this);
            args.push_back(valueStack.top());
            valueStack.pop();
        }
        
        llvm::Value* result = builder->CreateCall(func, args);
        valueStack.push(result);
    }
}

void CodeGenerator::visit(ExpressionStatement* node) {
    node->expression->accept(this);
    if (!valueStack.empty()) {
        valueStack.pop();  // Discard the result
    }
}

void CodeGenerator::visit(VariableDeclaration* node) {
    llvm::Value* value = nullptr;
    
    if (node->initializer) {
        node->initializer->accept(this);
        value = valueStack.top();
        valueStack.pop();
    } else {
        // Default to 0 or null
        value = llvm::ConstantFP::get(*context, llvm::APFloat(0.0));
    }
    
    if (currentFunction) {
        llvm::AllocaInst* alloca = createEntryBlockAlloca(currentFunction, node->name, value->getType());
        builder->CreateStore(value, alloca);
        symbolTables.back()[node->name] = alloca;
    }
}

void CodeGenerator::visit(BlockStatement* node) {
    pushScope();
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
    popScope();
}

void CodeGenerator::visit(IfStatement* node) {
    node->condition->accept(this);
    llvm::Value* condition = valueStack.top();
    valueStack.pop();
    
    condition = convertToBool(condition);
    
    llvm::Function* function = builder->GetInsertBlock()->getParent();
    
    llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(*context, "then", function);
    llvm::BasicBlock* elseBlock = nullptr;
    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context, "merge", function);
    
    if (node->elseStatement) {
        elseBlock = llvm::BasicBlock::Create(*context, "else", function);
        builder->CreateCondBr(condition, thenBlock, elseBlock);
    } else {
        builder->CreateCondBr(condition, thenBlock, mergeBlock);
    }
    
    // Then block
    builder->SetInsertPoint(thenBlock);
    node->thenStatement->accept(this);
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBlock);
    }
    
    // Else block
    if (node->elseStatement) {
        builder->SetInsertPoint(elseBlock);
        node->elseStatement->accept(this);
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(mergeBlock);
        }
    }
    
    // Merge block
    builder->SetInsertPoint(mergeBlock);
}

void CodeGenerator::visit(WhileStatement* node) {
    llvm::Function* function = builder->GetInsertBlock()->getParent();
    
    llvm::BasicBlock* condBlock = llvm::BasicBlock::Create(*context, "while.cond", function);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(*context, "while.body", function);
    llvm::BasicBlock* endBlock = llvm::BasicBlock::Create(*context, "while.end", function);
    
    builder->CreateBr(condBlock);
    
    // Condition block
    builder->SetInsertPoint(condBlock);
    node->condition->accept(this);
    llvm::Value* condition = valueStack.top();
    valueStack.pop();
    condition = convertToBool(condition);
    builder->CreateCondBr(condition, bodyBlock, endBlock);
    
    // Body block
    builder->SetInsertPoint(bodyBlock);
    node->body->accept(this);
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(condBlock);
    }
    
    // End block
    builder->SetInsertPoint(endBlock);
}

void CodeGenerator::visit(ForStatement* node) {
    llvm::Function* function = builder->GetInsertBlock()->getParent();
    
    // Init
    if (node->init) {
        node->init->accept(this);
    }
    
    llvm::BasicBlock* condBlock = llvm::BasicBlock::Create(*context, "for.cond", function);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(*context, "for.body", function);
    llvm::BasicBlock* updateBlock = llvm::BasicBlock::Create(*context, "for.update", function);
    llvm::BasicBlock* endBlock = llvm::BasicBlock::Create(*context, "for.end", function);
    
    builder->CreateBr(condBlock);
    
    // Condition block
    builder->SetInsertPoint(condBlock);
    if (node->condition) {
        node->condition->accept(this);
        llvm::Value* condition = valueStack.top();
        valueStack.pop();
        condition = convertToBool(condition);
        builder->CreateCondBr(condition, bodyBlock, endBlock);
    } else {
        builder->CreateBr(bodyBlock);
    }
    
    // Body block
    builder->SetInsertPoint(bodyBlock);
    node->body->accept(this);
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(updateBlock);
    }
    
    // Update block
    builder->SetInsertPoint(updateBlock);
    if (node->update) {
        node->update->accept(this);
        if (!valueStack.empty()) {
            valueStack.pop();  // Discard result
        }
    }
    builder->CreateBr(condBlock);
    
    // End block
    builder->SetInsertPoint(endBlock);
}

void CodeGenerator::visit(ReturnStatement* node) {
    if (node->value) {
        node->value->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        builder->CreateRet(value);
    } else {
        builder->CreateRetVoid();
    }
}

void CodeGenerator::visit(FunctionDeclaration* node) {
    // For now, we're not implementing user-defined functions
    // This would require more complex handling
    throw std::runtime_error("User-defined functions not yet implemented");
}

void CodeGenerator::declareStrlen() {
    if (module->getFunction("strlen")) return;
    
    std::vector<llvm::Type*> params = {
        llvm::PointerType::getUnqual(*context)  // const char*
    };
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::Type::getInt64Ty(*context),  // size_t (64-bit)
        params,
        false  // not variadic
    );
    
    llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "strlen", *module);
}

void CodeGenerator::declareMalloc() {
    if (module->getFunction("malloc")) return;
    
    std::vector<llvm::Type*> params = {
        llvm::Type::getInt64Ty(*context)  // size_t
    };
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),  // void*
        params,
        false  // not variadic
    );
    
    llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "malloc", *module);
}

void CodeGenerator::declareStrcpy() {
    if (module->getFunction("strcpy")) return;
    
    std::vector<llvm::Type*> params = {
        llvm::PointerType::getUnqual(*context),  // char* dest
        llvm::PointerType::getUnqual(*context)   // const char* src
    };
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),  // char* (returns dest)
        params,
        false  // not variadic
    );
    
    llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "strcpy", *module);
}

void CodeGenerator::declareStrcat() {
    if (module->getFunction("strcat")) return;
    
    std::vector<llvm::Type*> params = {
        llvm::PointerType::getUnqual(*context),  // char* dest
        llvm::PointerType::getUnqual(*context)   // const char* src
    };
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),  // char* (returns dest)
        params,
        false  // not variadic
    );
    
    llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "strcat", *module);
}

llvm::Value* CodeGenerator::convertToString(llvm::Value* value) {
    // If it's already a string, return as-is
    if (value->getType()->isPointerTy()) {
        return value;
    }
    
    // Convert to double if needed
    if (!value->getType()->isDoubleTy()) {
        value = convertToDouble(value);
    }
    
    // Allocate buffer for string result (32 characters should be enough for most numbers)
    llvm::Type* charType = llvm::Type::getInt8Ty(*context);
    llvm::Type* arrayType = llvm::ArrayType::get(charType, 32);
    llvm::AllocaInst* buffer = builder->CreateAlloca(arrayType, nullptr, "str_buffer");
    
    // Get pointer to first element of array
    std::vector<llvm::Value*> indices = {
        llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
        llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
    };
    llvm::Value* bufferPtr = builder->CreateInBoundsGEP(
        arrayType,
        buffer,
        indices
    );
    
    // Create format string for snprintf ("%g" for general format)
    llvm::GlobalVariable* format = builder->CreateGlobalString("%g");
    std::vector<llvm::Value*> formatIndices = {
        llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
        llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
    };
    llvm::Value* formatPtr = builder->CreateInBoundsGEP(
        format->getValueType(),
        format,
        formatIndices
    );
    
    // Call snprintf to convert number to string
    llvm::Function* snprintfFunc = module->getFunction("snprintf");
    builder->CreateCall(snprintfFunc, {
        bufferPtr,
        llvm::ConstantInt::get(*context, llvm::APInt(32, 32)),  // buffer size
        formatPtr,
        value
    });
    
    return bufferPtr;
}

llvm::Value* CodeGenerator::createStringConcatenation(llvm::Value* left, llvm::Value* right) {
    // Declare necessary C library functions if not already declared
    declareStrlen();
    declareMalloc();
    declareStrcpy();
    declareStrcat();
    
    // Convert non-string operands to strings
    if (!left->getType()->isPointerTy()) {
        left = convertToString(left);
    }
    if (!right->getType()->isPointerTy()) {
        right = convertToString(right);
    }
    
    // Get strlen function
    llvm::Function* strlenFunc = module->getFunction("strlen");
    
    // Calculate lengths of both strings
    llvm::Value* leftLen = builder->CreateCall(strlenFunc, {left}, "leftlen");
    llvm::Value* rightLen = builder->CreateCall(strlenFunc, {right}, "rightlen");
    
    // Calculate total length (left + right + 1 for null terminator)
    llvm::Value* totalLen = builder->CreateAdd(leftLen, rightLen, "addlen");
    totalLen = builder->CreateAdd(totalLen, llvm::ConstantInt::get(*context, llvm::APInt(64, 1)), "totallen");
    
    // Allocate memory for result string
    llvm::Function* mallocFunc = module->getFunction("malloc");
    llvm::Value* resultPtr = builder->CreateCall(mallocFunc, {totalLen}, "result");
    
    // Cast to char*
    llvm::Type* charPtrType = llvm::PointerType::getUnqual(*context);
    resultPtr = builder->CreateBitCast(resultPtr, charPtrType, "resultstr");
    
    // Copy left string to result
    llvm::Function* strcpyFunc = module->getFunction("strcpy");
    builder->CreateCall(strcpyFunc, {resultPtr, left});
    
    // Concatenate right string to result
    llvm::Function* strcatFunc = module->getFunction("strcat");
    builder->CreateCall(strcatFunc, {resultPtr, right});
    
    return resultPtr;
}