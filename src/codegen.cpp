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
    
    pushScope();
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
    for (auto it = symbolTables.rbegin(); it != symbolTables.rend(); ++it) {
        auto var = it->find(name);
        if (var != it->end()) {
            return builder->CreateLoad(var->second->getAllocatedType(), var->second, name);
        }
    }
    return nullptr;
}

void CodeGenerator::setVariable(const std::string& name, llvm::Value* value) {
    for (auto it = symbolTables.rbegin(); it != symbolTables.rend(); ++it) {
        auto var = it->find(name);
        if (var != it->end()) {
            llvm::Type* allocatedType = var->second->getAllocatedType();
            if (value->getType() == allocatedType) {
            builder->CreateStore(value, var->second);
            } else {
                llvm::AllocaInst* newAlloca = createEntryBlockAlloca(currentFunction, name + "_new", value->getType());
                builder->CreateStore(value, newAlloca);

                var->second = newAlloca;
            }
            return;
        }
    }
    
    if (currentFunction) {
        llvm::AllocaInst* alloca = createEntryBlockAlloca(currentFunction, name, value->getType());
        builder->CreateStore(value, alloca);
        symbolTables.back()[name] = alloca;
    }
}

void CodeGenerator::declareBuiltinFunctions() {
    declarePrintf();
    declareScanf();
    
    declareFgets();
    declareStdin();
    
    declareSnprintf();
    declareAtof();
    declareAtoi();
    
    declareFabs();
    declareRound();
    declarePow();
    declareSqrt();
    
    declareRand();
    declareSrand();
    
    declareStrlen();
    declareMalloc();
    declareStrcpy();
    declareStrcat();
    declareStrstr();
    
    std::vector<llvm::Type*> printParams = {
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* printType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(*context),
        printParams,
        true
    );
    
    llvm::Function* printFunc = llvm::Function::Create(
        printType,
        llvm::Function::ExternalLinkage,
        "print",
        module.get()
    );
    
    functions["print"] = printFunc;
    
    std::vector<llvm::Type*> inputParams = {};
    
    llvm::FunctionType* inputType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),
        inputParams,
        false
    );
    
    llvm::Function* inputFunc = llvm::Function::Create(
        inputType,
        llvm::Function::ExternalLinkage,
        "input",
        module.get()
    );
    
    functions["input"] = inputFunc;
    
    std::vector<llvm::Type*> strParams = {
        llvm::Type::getDoubleTy(*context)
    };
    
    llvm::FunctionType* strType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),
        strParams,
        false
    );
    
    llvm::Function* strFunc = llvm::Function::Create(
        strType,
        llvm::Function::ExternalLinkage,
        "str",
        module.get()
    );
    
    functions["str"] = strFunc;
    
    std::vector<llvm::Type*> numParams = {
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* numType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        numParams,
        false
    );
    
    llvm::Function* numFunc = llvm::Function::Create(
        numType,
        llvm::Function::ExternalLinkage,
        "num",
        module.get()
    );
    
    functions["num"] = numFunc;
    
    std::vector<llvm::Type*> intParams = {
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* intType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        intParams,
        false
    );
    
    llvm::Function* intFunc = llvm::Function::Create(
        intType,
        llvm::Function::ExternalLinkage,
        "int",
        module.get()
    );
    
    functions["int"] = intFunc;
    
    std::vector<llvm::Type*> absParams = {
        llvm::Type::getDoubleTy(*context)
    };
    
    llvm::FunctionType* absType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        absParams,
        false
    );
    
    llvm::Function* absFunc = llvm::Function::Create(
        absType,
        llvm::Function::ExternalLinkage,
        "abs",
        module.get()
    );
    
    functions["abs"] = absFunc;
    
    std::vector<llvm::Type*> roundParams = {
        llvm::Type::getDoubleTy(*context)
    };
    
    llvm::FunctionType* roundType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        roundParams,
        true
    );
    
    llvm::Function* roundFunc = llvm::Function::Create(
        roundType,
        llvm::Function::ExternalLinkage,
        "round",
        module.get()
    );
    
    functions["round"] = roundFunc;
    
    std::vector<llvm::Type*> minParams = {
        llvm::Type::getDoubleTy(*context),
        llvm::Type::getDoubleTy(*context)
    };
    
    llvm::FunctionType* minType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        minParams,
        true
    );
    
    llvm::Function* minFunc = llvm::Function::Create(
        minType,
        llvm::Function::ExternalLinkage,
        "min",
        module.get()
    );
    
    functions["min"] = minFunc;
    
    std::vector<llvm::Type*> maxParams = {
        llvm::Type::getDoubleTy(*context),
        llvm::Type::getDoubleTy(*context)
    };
    
    llvm::FunctionType* maxType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        maxParams,
        true
    );
    
    llvm::Function* maxFunc = llvm::Function::Create(
        maxType,
        llvm::Function::ExternalLinkage,
        "max",
        module.get()
    );
    
    functions["max"] = maxFunc;
    
    std::vector<llvm::Type*> powParams = {
        llvm::Type::getDoubleTy(*context),
        llvm::Type::getDoubleTy(*context)
    };
    
    llvm::FunctionType* powType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        powParams,
        false
    );
    
    llvm::Function* powFunc = llvm::Function::Create(
        powType,
        llvm::Function::ExternalLinkage,
        "pow",
        module.get()
    );
    
    functions["pow"] = powFunc;
    
    std::vector<llvm::Type*> sqrtParams = {
        llvm::Type::getDoubleTy(*context)
    };
    
    llvm::FunctionType* sqrtType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        sqrtParams,
        false
    );
    
    llvm::Function* sqrtFunc = llvm::Function::Create(
        sqrtType,
        llvm::Function::ExternalLinkage,
        "sqrt",
        module.get()
    );
    
    functions["sqrt"] = sqrtFunc;
    
    std::vector<llvm::Type*> randomParams = {};
    
    llvm::FunctionType* randomType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        randomParams,
        false
    );
    
    llvm::Function* randomFunc = llvm::Function::Create(
        randomType,
        llvm::Function::ExternalLinkage,
        "random",
        module.get()
    );
    
    functions["random"] = randomFunc;
}

llvm::Function* CodeGenerator::declarePrintf() {
    std::vector<llvm::Type*> printfParams = {
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* printfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),
        printfParams,
        true
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
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* scanfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),
        scanfParams,
        true
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
    std::vector<llvm::Type*> fgetsParams = {
        llvm::PointerType::getUnqual(*context),
        llvm::Type::getInt32Ty(*context),
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* fgetsType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),
        fgetsParams,
        false
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
    llvm::FunctionType* iobFuncType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),
        {},
        false
    );
    
    llvm::Function* getStdinFunc = llvm::Function::Create(
        iobFuncType,
        llvm::Function::ExternalLinkage,
        "get_stdin_ptr",
        module.get()
    );
    
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", getStdinFunc);
    llvm::IRBuilder<> tmpBuilder(entry);
    llvm::FunctionType* acrtIobFuncType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),
        {llvm::Type::getInt32Ty(*context)},
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
    
    llvm::Value* zero = llvm::ConstantInt::get(*context, llvm::APInt(32, 0));
    llvm::Value* stdinPtr = tmpBuilder.CreateCall(acrtIobFunc, {zero});
    tmpBuilder.CreateRet(stdinPtr);
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
    llvm::Type* fileType = llvm::PointerType::getUnqual(*context);
    
    llvm::GlobalVariable* stdinVar = new llvm::GlobalVariable(
        *module,
        fileType,
        false,
        llvm::GlobalValue::ExternalLinkage,
        nullptr,
        "stdin"
    );
    
    return stdinVar;
#endif
}

llvm::Function* CodeGenerator::declareSnprintf() {
    std::vector<llvm::Type*> snprintfParams = {
        llvm::PointerType::getUnqual(*context),
        llvm::Type::getInt64Ty(*context),
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* snprintfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),
        snprintfParams,
        true
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
    std::vector<llvm::Type*> atofParams = {
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* atofType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        atofParams,
        false
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
    std::vector<llvm::Type*> atoiParams = {
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* atoiType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),
        atoiParams,
        false
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
    std::vector<llvm::Type*> fabsParams = {
        llvm::Type::getDoubleTy(*context)
    };
    
    llvm::FunctionType* fabsType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        fabsParams,
        false
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
    std::vector<llvm::Type*> roundParams = {
        llvm::Type::getDoubleTy(*context)
    };
    
    llvm::FunctionType* roundType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        roundParams,
        false
    );
    
    llvm::Function* roundFunc = llvm::Function::Create(
        roundType,
        llvm::Function::ExternalLinkage,
        "round",
        module.get()
    );
    
    functions["mathRound"] = roundFunc;
    return roundFunc;
}

llvm::Function* CodeGenerator::declarePow() {
    std::vector<llvm::Type*> powParams = {
        llvm::Type::getDoubleTy(*context),
        llvm::Type::getDoubleTy(*context)
    };
    
    llvm::FunctionType* powType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        powParams,
        false
    );
    
    llvm::Function* powFunc = llvm::Function::Create(
        powType,
        llvm::Function::ExternalLinkage,
        "pow",
        module.get()
    );
    
    functions["mathPow"] = powFunc;
    return powFunc;
}

llvm::Function* CodeGenerator::declareSqrt() {
    std::vector<llvm::Type*> sqrtParams = {
        llvm::Type::getDoubleTy(*context)
    };
    
    llvm::FunctionType* sqrtType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        sqrtParams,
        false
    );
    
    llvm::Function* sqrtFunc = llvm::Function::Create(
        sqrtType,
        llvm::Function::ExternalLinkage,
        "sqrt",
        module.get()
    );
    
    functions["mathSqrt"] = sqrtFunc;
    return sqrtFunc;
}

llvm::Function* CodeGenerator::declareRand() {
    std::vector<llvm::Type*> randParams = {};
    
    llvm::FunctionType* randType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),
        randParams,
        false
    );
    
    llvm::Function* randFunc = llvm::Function::Create(
        randType,
        llvm::Function::ExternalLinkage,
        "rand",
        module.get()
    );
    
    functions["rand"] = randFunc;
    return randFunc;
}

llvm::Function* CodeGenerator::declareSrand() {
    std::vector<llvm::Type*> srandParams = {
        llvm::Type::getInt32Ty(*context)
    };
    
    llvm::FunctionType* srandType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(*context),
        srandParams,
        false
    );
    
    llvm::Function* srandFunc = llvm::Function::Create(
        srandType,
        llvm::Function::ExternalLinkage,
        "srand",
        module.get()
    );
    
    functions["srand"] = srandFunc;
    return srandFunc;
}

llvm::Function* CodeGenerator::declarePuts() {
    std::vector<llvm::Type*> putsParams = {
        llvm::PointerType::getUnqual(*context)
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

llvm::Value* CodeGenerator::createFormatString(const std::string& format) {
    llvm::GlobalVariable* formatVar = builder->CreateGlobalString(format);
    std::vector<llvm::Value*> indices = {getInt32(0), getInt32(0)};
    return builder->CreateInBoundsGEP(formatVar->getValueType(), formatVar, indices);
}

llvm::Value* CodeGenerator::getInt32(int32_t value) {
    return llvm::ConstantInt::get(*context, llvm::APInt(32, value));
}

llvm::Value* CodeGenerator::isStringPointer(llvm::Value* ptrValue, bool allowEmptyStrings) {
    llvm::Value* firstByte = builder->CreateLoad(llvm::Type::getInt8Ty(*context), 
        builder->CreateBitCast(ptrValue, llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*context))));
    
    static const int32_t PRINTABLE_MIN = 32;
    static const int32_t PRINTABLE_MAX = 126;
    
    llvm::Value* isPrintable = builder->CreateAnd(
        builder->CreateICmpUGE(firstByte, llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), PRINTABLE_MIN)),
        builder->CreateICmpULE(firstByte, llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), PRINTABLE_MAX))
    );
    
    if (allowEmptyStrings) {
        llvm::Value* isNullTerminator = builder->CreateICmpEQ(firstByte, 
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 0));
        return builder->CreateOr(isNullTerminator, isPrintable);
    }
    
    return isPrintable;
}

llvm::Value* CodeGenerator::convertToDouble(llvm::Value* value) {
    if (value->getType()->isDoubleTy()) {
        return value;
    } else if (value->getType()->isIntegerTy()) {
        return builder->CreateSIToFP(value, llvm::Type::getDoubleTy(*context), "cast");
    } else if (value->getType()->isPointerTy()) {
        return unboxValue(value, llvm::Type::getDoubleTy(*context));
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

llvm::Value* CodeGenerator::getInt64(int64_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), value);
}

llvm::Value* CodeGenerator::boxValue(llvm::Value* value) {
    if (!value) return llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context));
    
    if (value->getType()->isPointerTy()) {
        // Already a pointer, return as-is
        return value;
    } else if (value->getType()->isDoubleTy()) {
        // Box double by allocating memory and storing the value
        llvm::Function* mallocFunc = module->getFunction("malloc");
        if (!mallocFunc) {
            declareMalloc();
            mallocFunc = module->getFunction("malloc");
        }
        
        llvm::Value* size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 8); // sizeof(double)
        llvm::Value* ptr = builder->CreateCall(mallocFunc, {size});
        llvm::Value* doublePtr = builder->CreateBitCast(ptr, llvm::PointerType::getUnqual(llvm::Type::getDoubleTy(*context)));
        builder->CreateStore(value, doublePtr);
        return builder->CreateBitCast(doublePtr, llvm::PointerType::getUnqual(*context));
    } else {
        llvm::Value* doubleVal = convertToDouble(value);
        return boxValue(doubleVal);
    }
}

llvm::Value* CodeGenerator::unboxPointerToDouble(llvm::Value* ptrValue) {
    llvm::Value* isString = isStringPointer(ptrValue, false);
    
    llvm::BasicBlock* stringBlock = llvm::BasicBlock::Create(*context, "string_to_double", currentFunction);
    llvm::BasicBlock* boxedBlock = llvm::BasicBlock::Create(*context, "unbox_double", currentFunction);
    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context, "unbox_merge", currentFunction);
    
    builder->CreateCondBr(isString, stringBlock, boxedBlock);
    
    builder->SetInsertPoint(stringBlock);
    llvm::Function* atofFunc = module->getFunction("atof");
    if (!atofFunc) {
        declareAtof();
        atofFunc = module->getFunction("atof");
    }
    llvm::Value* stringResult = builder->CreateCall(atofFunc, {ptrValue});
    builder->CreateBr(mergeBlock);
    
    builder->SetInsertPoint(boxedBlock);
    llvm::Value* doublePtr = builder->CreateBitCast(ptrValue, llvm::PointerType::getUnqual(llvm::Type::getDoubleTy(*context)));
    llvm::Value* boxedResult = builder->CreateLoad(llvm::Type::getDoubleTy(*context), doublePtr);
    builder->CreateBr(mergeBlock);
    
    builder->SetInsertPoint(mergeBlock);
    llvm::PHINode* result = builder->CreatePHI(llvm::Type::getDoubleTy(*context), 2);
    result->addIncoming(stringResult, stringBlock);
    result->addIncoming(boxedResult, boxedBlock);
    
    return result;
}

llvm::Value* CodeGenerator::unboxValue(llvm::Value* boxedValue, llvm::Type* expectedType) {
    if (!boxedValue || !expectedType) {
        return llvm::ConstantFP::get(*context, llvm::APFloat(0.0));
    }
    
    if (expectedType->isDoubleTy()) {
        if (boxedValue->getType()->isPointerTy()) {
            return unboxPointerToDouble(boxedValue);
        } else if (boxedValue->getType()->isIntegerTy()) {
            return builder->CreateSIToFP(boxedValue, llvm::Type::getDoubleTy(*context), "cast");
        }
        return boxedValue;
    } else if (expectedType->isPointerTy()) {
        return boxedValue;
    } else if (expectedType->isIntegerTy()) {
        llvm::Value* doubleVal = unboxValue(boxedValue, llvm::Type::getDoubleTy(*context));
        return builder->CreateFPToSI(doubleVal, expectedType);
    }
    return boxedValue;
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
        
        program->accept(this);
        builder->CreateRet(llvm::ConstantInt::get(*context, llvm::APInt(32, 0)));
        
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

void CodeGenerator::visit(Program* node) {
    for (auto& stmt : node->statements) {
        if (auto* funcDecl = dynamic_cast<FunctionDeclaration*>(stmt.get())) {
            std::vector<llvm::Type*> paramTypes;
            for (size_t i = 0; i < funcDecl->parameters.size(); i++) {
                paramTypes.push_back(llvm::Type::getDoubleTy(*context));
            }
            
            llvm::FunctionType* funcType = llvm::FunctionType::get(
                llvm::PointerType::getUnqual(*context),
                paramTypes,
                false
            );
            
            llvm::Function* function = llvm::Function::Create(
                funcType,
                llvm::Function::InternalLinkage,
                funcDecl->name,
                module.get()
            );
            
            functions[funcDecl->name] = function;
        }
    }
    
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
}

void CodeGenerator::visit(NumberLiteral* node) {
    valueStack.push(llvm::ConstantFP::get(*context, llvm::APFloat(node->value)));
}

void CodeGenerator::visit(StringLiteral* node) {
    llvm::Constant* strConstant = llvm::ConstantDataArray::getString(*context, node->value);
    
    llvm::GlobalVariable* globalStr = new llvm::GlobalVariable(
        *module,
        strConstant->getType(),
        true,
        llvm::GlobalValue::PrivateLinkage,
        strConstant,
        ".str"
    );
    
    std::vector<llvm::Value*> indices = {getInt32(0), getInt32(0)};
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
        if (left->getType()->isPointerTy() || right->getType()->isPointerTy()) {
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
        if (!node->arguments.empty()) {
            std::cerr << "Warning: input() function takes no arguments, ignoring provided arguments" << std::endl;
        }
        
        llvm::Type* charType = llvm::Type::getInt8Ty(*context);
        llvm::Type* arrayType = llvm::ArrayType::get(charType, 1024);
        llvm::AllocaInst* buffer = builder->CreateAlloca(arrayType, nullptr, "input_buffer");
        
        std::vector<llvm::Value*> indices = {
            llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
        };
        llvm::Value* bufferPtr = builder->CreateInBoundsGEP(
            arrayType,
            buffer,
            indices
        );
        
        llvm::Value* stdinPtr;
#ifdef _WIN32
        llvm::Function* getStdinFunc = module->getFunction("get_stdin_ptr");
        if (!getStdinFunc) {
            // Recreate the helper if it doesn't exist
            declareStdin();
            getStdinFunc = module->getFunction("get_stdin_ptr");
        }
        stdinPtr = builder->CreateCall(getStdinFunc, {}, "stdin_ptr");
#else
        llvm::GlobalVariable* stdinVar = module->getGlobalVariable("stdin");
        if (!stdinVar) {
            stdinVar = declareStdin();
        }
        stdinPtr = builder->CreateLoad(llvm::PointerType::getUnqual(*context), stdinVar, "stdin_load");
#endif
        
        llvm::Value* bufferSize = llvm::ConstantInt::get(*context, llvm::APInt(32, 1024));
        llvm::Value* result = builder->CreateCall(functions["fgets"], {bufferPtr, bufferSize, stdinPtr});
        
        llvm::Function* strlenFunc = module->getFunction("strlen");
        if (!strlenFunc) {
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
        
        llvm::Value* one = llvm::ConstantInt::get(*context, llvm::APInt(64, 1));
        llvm::Value* lastCharIndex = builder->CreateSub(length, one);
        llvm::Value* lastCharPtr = builder->CreateInBoundsGEP(charType, bufferPtr, lastCharIndex);
        llvm::Value* lastChar = builder->CreateLoad(charType, lastCharPtr);
        llvm::Value* newline = llvm::ConstantInt::get(charType, 10);
        llvm::Value* isNewline = builder->CreateICmpEQ(lastChar, newline);
        llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(*context, "remove_newline", currentFunction);
        llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context, "merge", currentFunction);
        
        builder->CreateCondBr(isNewline, thenBlock, mergeBlock);
        
        builder->SetInsertPoint(thenBlock);
        llvm::Value* nullChar = llvm::ConstantInt::get(charType, 0);
        builder->CreateStore(nullChar, lastCharPtr);
        builder->CreateBr(mergeBlock);
        
        builder->SetInsertPoint(mergeBlock);
        valueStack.push(bufferPtr);
        return;
    } else if (node->name == "str") {
        if (node->arguments.size() != 1) {
            std::cerr << "Error: str() expects exactly 1 argument" << std::endl;
            return;
        }
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        if (!value->getType()->isDoubleTy()) {
            value = convertToDouble(value);
        }
        llvm::Type* charType = llvm::Type::getInt8Ty(*context);
        llvm::Type* arrayType = llvm::ArrayType::get(charType, 32);
        llvm::AllocaInst* buffer = builder->CreateAlloca(arrayType, nullptr, "str_buffer");
        
        std::vector<llvm::Value*> indices = {
            llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
        };
        llvm::Value* bufferPtr = builder->CreateInBoundsGEP(
            arrayType,
            buffer,
            indices
        );
        
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
        
        llvm::Value* bufferSize = llvm::ConstantInt::get(*context, llvm::APInt(64, 32));
        builder->CreateCall(functions["snprintf"], {bufferPtr, bufferSize, formatPtr, value});
        
        valueStack.push(bufferPtr);
        return;
    } else if (node->name == "num") {
        if (node->arguments.size() != 1) {
            std::cerr << "Error: num() expects exactly 1 argument" << std::endl;
            return;
        }
        
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        if (!value->getType()->isPointerTy()) {
            std::cerr << "Error: num() expects a string argument" << std::endl;
            return;
        }
        
        llvm::Value* result = builder->CreateCall(functions["atof"], {value});
        valueStack.push(result);
        return;
    } else if (node->name == "int") {
        if (node->arguments.size() != 1) {
            std::cerr << "Error: int() expects exactly 1 argument" << std::endl;
            return;
        }
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        if (!value->getType()->isPointerTy()) {
            std::cerr << "Error: int() expects a string argument" << std::endl;
            return;
        }
        
        llvm::Value* intResult = builder->CreateCall(functions["atoi"], {value});
        
        llvm::Value* result = builder->CreateSIToFP(intResult, llvm::Type::getDoubleTy(*context));
        valueStack.push(result);
        return;
    } else if (node->name == "abs") {
        if (node->arguments.size() != 1) {
            std::cerr << "Error: abs() expects exactly 1 argument" << std::endl;
            return;
        }
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        if (!value->getType()->isDoubleTy()) {
            value = convertToDouble(value);
        }
        
        llvm::Value* result = builder->CreateCall(functions["fabs"], {value});
        valueStack.push(result);
        return;
    } else if (node->name == "round") {
        if (node->arguments.size() < 1 || node->arguments.size() > 2) {
            std::cerr << "Error: round() expects 1 or 2 arguments" << std::endl;
            return;
        }
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        if (!value->getType()->isDoubleTy()) {
            value = convertToDouble(value);
        }
        
        if (node->arguments.size() == 1) {
            llvm::Value* result = builder->CreateCall(functions["mathRound"], {value});
            valueStack.push(result);
        } else {
            node->arguments[1]->accept(this);
            llvm::Value* decimalPlaces = valueStack.top();
            valueStack.pop();
            
            if (!decimalPlaces->getType()->isDoubleTy()) {
                decimalPlaces = convertToDouble(decimalPlaces);
            }
            llvm::Value* ten = llvm::ConstantFP::get(*context, llvm::APFloat(10.0));
            llvm::Value* scaleFactor = builder->CreateCall(functions["mathPow"], {ten, decimalPlaces});
            
            llvm::Value* scaled = builder->CreateFMul(value, scaleFactor);
            llvm::Value* roundedScaled = builder->CreateCall(functions["mathRound"], {scaled});
            llvm::Value* result = builder->CreateFDiv(roundedScaled, scaleFactor);
            
            valueStack.push(result);
        }
        return;
    } else if (node->name == "min") {
        if (node->arguments.size() < 2) {
            std::cerr << "Error: min() expects at least 2 arguments" << std::endl;
            return;
        }
        node->arguments[0]->accept(this);
        llvm::Value* minValue = valueStack.top();
        valueStack.pop();
        
        if (!minValue->getType()->isDoubleTy()) {
            minValue = convertToDouble(minValue);
        }
        for (size_t i = 1; i < node->arguments.size(); i++) {
            node->arguments[i]->accept(this);
            llvm::Value* currentValue = valueStack.top();
            valueStack.pop();
            
            if (!currentValue->getType()->isDoubleTy()) {
                currentValue = convertToDouble(currentValue);
            }
            
            llvm::Value* cmp = builder->CreateFCmpOLT(currentValue, minValue);
            minValue = builder->CreateSelect(cmp, currentValue, minValue);
        }
        
        valueStack.push(minValue);
        return;
    } else if (node->name == "max") {
        if (node->arguments.size() < 2) {
            std::cerr << "Error: max() expects at least 2 arguments" << std::endl;
            return;
        }
        
        node->arguments[0]->accept(this);
        llvm::Value* maxValue = valueStack.top();
        valueStack.pop();
        
        if (!maxValue->getType()->isDoubleTy()) {
            maxValue = convertToDouble(maxValue);
        }
        for (size_t i = 1; i < node->arguments.size(); i++) {
            node->arguments[i]->accept(this);
            llvm::Value* currentValue = valueStack.top();
            valueStack.pop();
            
            if (!currentValue->getType()->isDoubleTy()) {
                currentValue = convertToDouble(currentValue);
            }
            
            llvm::Value* cmp = builder->CreateFCmpOGT(currentValue, maxValue);
            maxValue = builder->CreateSelect(cmp, currentValue, maxValue);
        }
        
        valueStack.push(maxValue);
        return;
    } else if (node->name == "pow") {
        if (node->arguments.size() != 2) {
            std::cerr << "Error: pow() expects exactly 2 arguments" << std::endl;
            return;
        }
        
        node->arguments[0]->accept(this);
        llvm::Value* base = valueStack.top();
        valueStack.pop();
        
        if (!base->getType()->isDoubleTy()) {
            base = convertToDouble(base);
        }
        node->arguments[1]->accept(this);
        llvm::Value* exponent = valueStack.top();
        valueStack.pop();
        
        if (!exponent->getType()->isDoubleTy()) {
            exponent = convertToDouble(exponent);
        }
        
        llvm::Value* result = builder->CreateCall(functions["mathPow"], {base, exponent});
        valueStack.push(result);
        return;
    } else if (node->name == "sqrt") {
        if (node->arguments.size() != 1) {
            std::cerr << "Error: sqrt() expects exactly 1 argument" << std::endl;
            return;
        }
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        if (!value->getType()->isDoubleTy()) {
            value = convertToDouble(value);
        }
        
        llvm::Value* result = builder->CreateCall(functions["mathSqrt"], {value});
        valueStack.push(result);
        return;
    } else if (node->name == "random") {
        if (!node->arguments.empty()) {
            std::cerr << "Warning: random() function takes no arguments, ignoring provided arguments" << std::endl;
        }
        llvm::GlobalVariable* stateVar = module->getGlobalVariable("_random_state");
        if (!stateVar) {
            stateVar = new llvm::GlobalVariable(
                *module,
                llvm::Type::getInt64Ty(*context),
                false,
                llvm::GlobalValue::InternalLinkage,
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1),
                "_random_state"
            );
        }
        llvm::GlobalVariable* seededVar = module->getGlobalVariable("_random_seeded");
        if (!seededVar) {
            seededVar = new llvm::GlobalVariable(
                *module,
                llvm::Type::getInt1Ty(*context),
                false,
                llvm::GlobalValue::InternalLinkage,
                llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), 0),
                "_random_seeded"
            );
        }
        llvm::Value* alreadySeeded = builder->CreateLoad(
            llvm::Type::getInt1Ty(*context), 
            seededVar, 
            "seeded_check"
        );
        
        llvm::BasicBlock* seedBlock = llvm::BasicBlock::Create(*context, "seed_random", currentFunction);
        llvm::BasicBlock* skipSeedBlock = llvm::BasicBlock::Create(*context, "skip_seed", currentFunction);
        llvm::BasicBlock* afterSeedBlock = llvm::BasicBlock::Create(*context, "after_seed", currentFunction);
        
        builder->CreateCondBr(alreadySeeded, skipSeedBlock, seedBlock);
        
        builder->SetInsertPoint(seedBlock);
        if (!module->getFunction("time")) {
            std::vector<llvm::Type*> timeParams = {
                llvm::PointerType::getUnqual(*context) // time_t *
            };
            
            llvm::FunctionType* timeType = llvm::FunctionType::get(
                llvm::Type::getInt64Ty(*context),
                timeParams,
                false
            );
            
            llvm::Function::Create(
                timeType,
                llvm::Function::ExternalLinkage,
                "time",
                module.get()
            );
        }
        
        llvm::Value* nullPtr = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context));
        llvm::Value* currentTime = builder->CreateCall(module->getFunction("time"), {nullPtr});
        llvm::AllocaInst* localVar = builder->CreateAlloca(llvm::Type::getInt32Ty(*context), nullptr, "entropy");
        llvm::Value* stackAddr = builder->CreatePtrToInt(localVar, llvm::Type::getInt64Ty(*context));
        llvm::Value* multiplier1 = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1103515245ULL);
        llvm::Value* multiplier2 = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 12345ULL);
        llvm::Value* increment = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1);
        llvm::Value* shift16 = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 16);
        
        llvm::Value* timePart = builder->CreateMul(currentTime, multiplier1);
        llvm::Value* addrPart = builder->CreateMul(stackAddr, multiplier2);
        llvm::Value* combined = builder->CreateAdd(timePart, addrPart);
        combined = builder->CreateAdd(combined, increment);
        
        llvm::Value* addrShifted = builder->CreateLShr(stackAddr, shift16);
        llvm::Value* finalSeed = builder->CreateXor(combined, addrShifted);
        
        builder->CreateStore(finalSeed, stateVar);
        builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), 1), seededVar);
        
        builder->CreateBr(afterSeedBlock);
        
        builder->SetInsertPoint(skipSeedBlock);
        builder->CreateBr(afterSeedBlock);
        
        builder->SetInsertPoint(afterSeedBlock);
        llvm::Value* state = builder->CreateLoad(llvm::Type::getInt64Ty(*context), stateVar, "current_state");
        llvm::Value* a = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1664525ULL);
        llvm::Value* c = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1013904223ULL);
        
        llvm::Value* nextState = builder->CreateMul(state, a);
        nextState = builder->CreateAdd(nextState, c);
        
        builder->CreateStore(nextState, stateVar);
        llvm::Value* shift32 = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 32);
        llvm::Value* upperBits = builder->CreateLShr(nextState, shift32);
        llvm::Value* randInt = builder->CreateTrunc(upperBits, llvm::Type::getInt32Ty(*context));
        
        llvm::Value* randDouble = builder->CreateUIToFP(randInt, llvm::Type::getDoubleTy(*context));
        
        llvm::Value* divisor = llvm::ConstantFP::get(*context, llvm::APFloat(4294967296.0));
        llvm::Value* result = builder->CreateFDiv(randDouble, divisor);
        valueStack.push(result);
        return;
    } else if (node->name == "len") {
        if (node->arguments.size() != 1) {
            throw std::runtime_error("len() expects exactly 1 argument");
        }

        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        if (!value->getType()->isPointerTy()) {
            throw std::runtime_error("len() expects a string or array argument");
        }
        llvm::Type* elementType = llvm::Type::getDoubleTy(*context);
        llvm::Value* sizePtr = builder->CreateInBoundsGEP(elementType, value,
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), -1));
        
        llvm::BasicBlock* arrayBlock = llvm::BasicBlock::Create(*context, "is_array", currentFunction);
        llvm::BasicBlock* stringBlock = llvm::BasicBlock::Create(*context, "is_string", currentFunction);
        llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context, "len_merge", currentFunction);
        
        llvm::Value* firstByte = builder->CreateLoad(llvm::Type::getInt8Ty(*context), 
            builder->CreateBitCast(value, llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*context))));
        llvm::Value* isPrintable = builder->CreateAnd(
            builder->CreateICmpUGE(firstByte, llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 32)),
            builder->CreateICmpULE(firstByte, llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 126))
        );
        
        builder->CreateCondBr(isPrintable, stringBlock, arrayBlock);
        
        builder->SetInsertPoint(arrayBlock);
        llvm::Value* arraySize = builder->CreateLoad(elementType, sizePtr);
        builder->CreateBr(mergeBlock);
        
        builder->SetInsertPoint(stringBlock);
        llvm::Function* strlenFunc = module->getFunction("strlen");
        if (!strlenFunc) {
            declareStrlen();
            strlenFunc = module->getFunction("strlen");
        }
        llvm::Value* strLength = builder->CreateCall(strlenFunc, {value});
        llvm::Value* strLengthDouble = builder->CreateUIToFP(strLength, elementType);
        builder->CreateBr(mergeBlock);
        
        builder->SetInsertPoint(mergeBlock);
        llvm::PHINode* result = builder->CreatePHI(elementType, 2);
        result->addIncoming(arraySize, arrayBlock);
        result->addIncoming(strLengthDouble, stringBlock);
        
        valueStack.push(result);
        return;
    } else if (node->name == "upper") {
        // Handle upper specially - converts string to uppercase
        if (node->arguments.size() != 1) {
            throw std::runtime_error("upper() expects exactly 1 argument");
        }
        
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        if (!value->getType()->isPointerTy()) {
            throw std::runtime_error("upper() expects a string argument");
        }
        
        llvm::Function* strlenFunc = module->getFunction("strlen");
        if (!strlenFunc) {
            declareStrlen();
            strlenFunc = module->getFunction("strlen");
        }
        
        llvm::Value* length = builder->CreateCall(strlenFunc, {value});
        
        // Allocate memory for the result string (length + 1 for null terminator)
        llvm::Function* mallocFunc = module->getFunction("malloc");
        if (!mallocFunc) {
            declareMalloc();
            mallocFunc = module->getFunction("malloc");
        }
        
        llvm::Value* lengthPlus1 = builder->CreateAdd(length, 
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1));
        llvm::Value* resultBuffer = builder->CreateCall(mallocFunc, {lengthPlus1});
        
        llvm::BasicBlock* loopCondBlock = llvm::BasicBlock::Create(*context, "loop_cond", currentFunction);
        llvm::BasicBlock* loopBodyBlock = llvm::BasicBlock::Create(*context, "loop_body", currentFunction);
        llvm::BasicBlock* loopEndBlock = llvm::BasicBlock::Create(*context, "loop_end", currentFunction);
        
        llvm::AllocaInst* indexVar = builder->CreateAlloca(llvm::Type::getInt64Ty(*context), nullptr, "index");
        builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 0), indexVar);
        
        builder->CreateBr(loopCondBlock);
        
        // index < length
        builder->SetInsertPoint(loopCondBlock);
        llvm::Value* currentIndex = builder->CreateLoad(llvm::Type::getInt64Ty(*context), indexVar, "current_index");
        llvm::Value* condition = builder->CreateICmpULT(currentIndex, length, "loop_condition");
        builder->CreateCondBr(condition, loopBodyBlock, loopEndBlock);
        
        builder->SetInsertPoint(loopBodyBlock);
        
        llvm::Value* srcPtr = builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*context), value, currentIndex);
        llvm::Value* srcChar = builder->CreateLoad(llvm::Type::getInt8Ty(*context), srcPtr, "src_char");
        
        // Check if character is lowercase (between 'a' and 'z')
        llvm::Value* charA = llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 97); // 'a'
        llvm::Value* charZ = llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 122); // 'z'
        llvm::Value* isLowercase = builder->CreateAnd(
            builder->CreateICmpUGE(srcChar, charA),
            builder->CreateICmpULE(srcChar, charZ)
        );
        
        // Convert to uppercase by subtracting 32 if it's lowercase
        llvm::Value* upperOffset = llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 32);
        llvm::Value* upperChar = builder->CreateSub(srcChar, upperOffset);
        llvm::Value* resultChar = builder->CreateSelect(isLowercase, upperChar, srcChar);
        
        // Store character in result buffer
        llvm::Value* dstPtr = builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*context), resultBuffer, currentIndex);
        builder->CreateStore(resultChar, dstPtr);
        
        llvm::Value* nextIndex = builder->CreateAdd(currentIndex, 
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1));
        builder->CreateStore(nextIndex, indexVar);
        builder->CreateBr(loopCondBlock);
        
        builder->SetInsertPoint(loopEndBlock);
        llvm::Value* nullTermPtr = builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*context), resultBuffer, length);
        builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 0), nullTermPtr);
        
        valueStack.push(resultBuffer);
        return;
    } else if (node->name == "lower") {
        // Handle lower specially - converts string to lowercase
        if (node->arguments.size() != 1) {
            throw std::runtime_error("lower() expects exactly 1 argument");
        }
        
        node->arguments[0]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        if (!value->getType()->isPointerTy()) {
            throw std::runtime_error("lower() expects a string argument");
        }
        
        llvm::Function* strlenFunc = module->getFunction("strlen");
        if (!strlenFunc) {
            declareStrlen();
            strlenFunc = module->getFunction("strlen");
        }
        
        llvm::Value* length = builder->CreateCall(strlenFunc, {value});
        
        // Allocate memory for the result string (length + 1 for null terminator)
        llvm::Function* mallocFunc = module->getFunction("malloc");
        if (!mallocFunc) {
            declareMalloc();
            mallocFunc = module->getFunction("malloc");
        }
        
        llvm::Value* lengthPlus1 = builder->CreateAdd(length, 
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1));
        llvm::Value* resultBuffer = builder->CreateCall(mallocFunc, {lengthPlus1});
        
        llvm::BasicBlock* loopCondBlock = llvm::BasicBlock::Create(*context, "loop_cond", currentFunction);
        llvm::BasicBlock* loopBodyBlock = llvm::BasicBlock::Create(*context, "loop_body", currentFunction);
        llvm::BasicBlock* loopEndBlock = llvm::BasicBlock::Create(*context, "loop_end", currentFunction);
        
        llvm::AllocaInst* indexVar = builder->CreateAlloca(llvm::Type::getInt64Ty(*context), nullptr, "index");
        builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 0), indexVar);
        
        builder->CreateBr(loopCondBlock);
        
        // index < length
        builder->SetInsertPoint(loopCondBlock);
        llvm::Value* currentIndex = builder->CreateLoad(llvm::Type::getInt64Ty(*context), indexVar, "current_index");
        llvm::Value* condition = builder->CreateICmpULT(currentIndex, length, "loop_condition");
        builder->CreateCondBr(condition, loopBodyBlock, loopEndBlock);
        
        builder->SetInsertPoint(loopBodyBlock);
        
        llvm::Value* srcPtr = builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*context), value, currentIndex);
        llvm::Value* srcChar = builder->CreateLoad(llvm::Type::getInt8Ty(*context), srcPtr, "src_char");
        
        // Check if character is uppercase (between 'A' and 'Z')
        llvm::Value* charA = llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 65); // 'A'
        llvm::Value* charZ = llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 90); // 'Z'
        llvm::Value* isUppercase = builder->CreateAnd(
            builder->CreateICmpUGE(srcChar, charA),
            builder->CreateICmpULE(srcChar, charZ)
        );
        
        // Convert to lowercase by adding 32 if it's uppercase
        llvm::Value* lowerOffset = llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 32);
        llvm::Value* lowerChar = builder->CreateAdd(srcChar, lowerOffset);
        llvm::Value* resultChar = builder->CreateSelect(isUppercase, lowerChar, srcChar);
        
        // Store character in result buffer
        llvm::Value* dstPtr = builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*context), resultBuffer, currentIndex);
        builder->CreateStore(resultChar, dstPtr);
        
        llvm::Value* nextIndex = builder->CreateAdd(currentIndex, 
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1));
        builder->CreateStore(nextIndex, indexVar);
        builder->CreateBr(loopCondBlock);
        
        builder->SetInsertPoint(loopEndBlock);
        llvm::Value* nullTermPtr = builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*context), resultBuffer, length);
        builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 0), nullTermPtr);
        
        valueStack.push(resultBuffer);
        return;
    } else if (node->name == "includes") {
        if (node->arguments.size() != 2) {
            throw std::runtime_error("includes() expects exactly 2 arguments");
        }
        
        node->arguments[0]->accept(this);
        llvm::Value* haystack = valueStack.top();
        valueStack.pop();

        node->arguments[1]->accept(this);
        llvm::Value* needle = valueStack.top();
        valueStack.pop();
        
        if (!haystack->getType()->isPointerTy()) {
            throw std::runtime_error("includes() expects first argument to be a string or array");
        }

        if (needle->getType()->isPointerTy()) {
            llvm::Function* strstrFunc = module->getFunction("strstr");
            if (!strstrFunc) {
                declareStrstr();
                strstrFunc = module->getFunction("strstr");
            }
            
            llvm::Value* result = builder->CreateCall(strstrFunc, {haystack, needle});
            llvm::Value* nullPtr = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context));
            llvm::Value* found = builder->CreateICmpNE(result, nullPtr);
            llvm::Value* doubleResult = builder->CreateUIToFP(found, llvm::Type::getDoubleTy(*context));
            valueStack.push(doubleResult);
        } else {
            llvm::Type* doubleType = llvm::Type::getDoubleTy(*context);
            llvm::Type* int64Type = llvm::Type::getInt64Ty(*context);
            needle = convertToDouble(needle);
            
            llvm::Value* sizePtr = builder->CreateInBoundsGEP(doubleType, haystack, getInt64(-1));
            llvm::Value* arraySize = builder->CreateLoad(doubleType, sizePtr);
            llvm::Value* sizeInt = builder->CreateFPToUI(arraySize, int64Type);
            
            llvm::BasicBlock* loopBlock = llvm::BasicBlock::Create(*context, "loop", currentFunction);
            llvm::BasicBlock* exitBlock = llvm::BasicBlock::Create(*context, "exit", currentFunction);
            
            llvm::AllocaInst* indexVar = createEntryBlockAlloca(currentFunction, "index", int64Type);
            llvm::Value* zero = llvm::ConstantInt::get(int64Type, 0);
            llvm::Value* one = llvm::ConstantInt::get(int64Type, 1);
            builder->CreateStore(zero, indexVar);
            builder->CreateBr(loopBlock);
            
            builder->SetInsertPoint(loopBlock);
            llvm::Value* index = builder->CreateLoad(int64Type, indexVar);
            llvm::Value* inBounds = builder->CreateICmpULT(index, sizeInt);
            
            llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(*context, "body", currentFunction);
            builder->CreateCondBr(inBounds, bodyBlock, exitBlock);
            
            builder->SetInsertPoint(bodyBlock);
            llvm::Value* elementPtr = builder->CreateInBoundsGEP(doubleType, haystack, index);
            llvm::Value* element = builder->CreateLoad(doubleType, elementPtr);
            llvm::Value* isEqual = builder->CreateFCmpOEQ(element, needle);
            
            llvm::Value* nextIndex = builder->CreateAdd(index, one);
            builder->CreateStore(nextIndex, indexVar);
            builder->CreateCondBr(isEqual, exitBlock, loopBlock);
            
            builder->SetInsertPoint(exitBlock);
            llvm::PHINode* result = builder->CreatePHI(doubleType, 2, "result");
            result->addIncoming(llvm::ConstantFP::get(doubleType, 0.0), loopBlock);
            result->addIncoming(llvm::ConstantFP::get(doubleType, 1.0), bodyBlock);
            
            valueStack.push(result);
        }
        return;
    } else if (node->name == "replace") {
        if (node->arguments.size() != 3) {
            throw std::runtime_error("replace() expects exactly 3 arguments");
        }
        
        node->arguments[0]->accept(this);
        llvm::Value* haystack = valueStack.top();
        valueStack.pop();
        
        node->arguments[1]->accept(this);
        llvm::Value* oldStr = valueStack.top();
        valueStack.pop();
        
        node->arguments[2]->accept(this);
        llvm::Value* newStr = valueStack.top();
        valueStack.pop();
        
        if (!haystack->getType()->isPointerTy() || !oldStr->getType()->isPointerTy() || !newStr->getType()->isPointerTy()) {
            throw std::runtime_error("replace() expects three string arguments");
        }
        
        llvm::Function* strstrFunc = module->getFunction("strstr");
        if (!strstrFunc) {
            declareStrstr();
            strstrFunc = module->getFunction("strstr");
        }
        
        llvm::Function* strlenFunc = module->getFunction("strlen");
        if (!strlenFunc) {
            declareStrlen();
            strlenFunc = module->getFunction("strlen");
        }
        
        llvm::Function* mallocFunc = module->getFunction("malloc");
        if (!mallocFunc) {
            declareMalloc();
            mallocFunc = module->getFunction("malloc");
        }
        
        llvm::Value* foundPtr = builder->CreateCall(strstrFunc, {haystack, oldStr});
        llvm::Value* nullPtr = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context));
        llvm::Value* found = builder->CreateICmpNE(foundPtr, nullPtr);
        
        llvm::BasicBlock* replaceBlock = llvm::BasicBlock::Create(*context, "do_replace", currentFunction);
        llvm::BasicBlock* noReplaceBlock = llvm::BasicBlock::Create(*context, "no_replace", currentFunction);
        llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context, "merge", currentFunction);
        
        builder->CreateCondBr(found, replaceBlock, noReplaceBlock);
        
        builder->SetInsertPoint(noReplaceBlock);
        llvm::Value* haystackLen = builder->CreateCall(strlenFunc, {haystack});
        llvm::Value* haystackLenPlus1 = builder->CreateAdd(haystackLen, 
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1));
        llvm::Value* originalCopy = builder->CreateCall(mallocFunc, {haystackLenPlus1});
        
        llvm::Function* strcpyFunc = module->getFunction("strcpy");
        if (!strcpyFunc) {
            declareStrcpy();
            strcpyFunc = module->getFunction("strcpy");
        }
        builder->CreateCall(strcpyFunc, {originalCopy, haystack});
        builder->CreateBr(mergeBlock);
        
        builder->SetInsertPoint(replaceBlock);
        
        llvm::Value* oldLen = builder->CreateCall(strlenFunc, {oldStr});
        llvm::Value* newLen = builder->CreateCall(strlenFunc, {newStr});
        llvm::Value* prefixLen = builder->CreatePtrToInt(foundPtr, llvm::Type::getInt64Ty(*context));
        llvm::Value* haystackInt = builder->CreatePtrToInt(haystack, llvm::Type::getInt64Ty(*context));
        prefixLen = builder->CreateSub(prefixLen, haystackInt);
        llvm::Value* suffixStart = builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*context), foundPtr, oldLen);
        llvm::Value* suffixLen = builder->CreateCall(strlenFunc, {suffixStart});
        llvm::Value* resultLen = builder->CreateAdd(prefixLen, newLen);
        resultLen = builder->CreateAdd(resultLen, suffixLen);
        resultLen = builder->CreateAdd(resultLen, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1));
        
        llvm::Value* resultBuffer = builder->CreateCall(mallocFunc, {resultLen});
        
        if (!module->getFunction("strncpy")) {
            std::vector<llvm::Type*> params = {
                llvm::PointerType::getUnqual(*context), // char* dest
                llvm::PointerType::getUnqual(*context), // const char* src
                llvm::Type::getInt64Ty(*context) // size_t n
            };
            llvm::FunctionType* funcType = llvm::FunctionType::get(
                llvm::PointerType::getUnqual(*context),
                params,
                false
            );
            llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "strncpy", *module);
        }
        
        llvm::Function* strncpyFunc = module->getFunction("strncpy");
        builder->CreateCall(strncpyFunc, {resultBuffer, haystack, prefixLen});
        llvm::Value* afterPrefix = builder->CreateInBoundsGEP(llvm::Type::getInt8Ty(*context), resultBuffer, prefixLen);
        builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 0), afterPrefix);
        llvm::Function* strcatFunc = module->getFunction("strcat");
        if (!strcatFunc) {
            declareStrcat();
            strcatFunc = module->getFunction("strcat");
        }
        builder->CreateCall(strcatFunc, {resultBuffer, newStr});
        builder->CreateCall(strcatFunc, {resultBuffer, suffixStart});
        
        builder->CreateBr(mergeBlock);
        
        builder->SetInsertPoint(mergeBlock);
        llvm::PHINode* resultPhi = builder->CreatePHI(llvm::PointerType::getUnqual(*context), 2, "replace_result");
        resultPhi->addIncoming(originalCopy, noReplaceBlock);
        resultPhi->addIncoming(resultBuffer, replaceBlock);
        
        valueStack.push(resultPhi);
        return;
    } else if (node->name == "append") {
        if (node->arguments.size() != 2) {
            throw std::runtime_error("append() expects exactly 2 arguments");
        }
        
        node->arguments[0]->accept(this);
        llvm::Value* arrayPtr = valueStack.top();
        valueStack.pop();
        
        node->arguments[1]->accept(this);
        llvm::Value* newValue = valueStack.top();
        valueStack.pop();
        
        if (!arrayPtr->getType()->isPointerTy()) {
            throw std::runtime_error("append() expects an array as first argument");
        }
        
        llvm::Type* elementType = llvm::Type::getDoubleTy(*context);
        newValue = convertToDouble(newValue);
        
        llvm::Value* sizePtr = builder->CreateInBoundsGEP(elementType, arrayPtr, getInt64(-1));
        llvm::Value* currentSize = builder->CreateLoad(elementType, sizePtr);
        llvm::Value* currentSizeInt = builder->CreateFPToUI(currentSize, llvm::Type::getInt64Ty(*context));
        
        llvm::Value* newSize = builder->CreateAdd(currentSizeInt, 
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1));
        llvm::Value* elementSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 8);
        llvm::Value* totalElements = builder->CreateAdd(newSize, 
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1));
        llvm::Value* totalSize = builder->CreateMul(totalElements, elementSize);
        
        llvm::Function* mallocFunc = module->getFunction("malloc");
        if (!mallocFunc) {
            declareMalloc();
            mallocFunc = module->getFunction("malloc");
        }
        
        llvm::Value* newArrayPtr = builder->CreateCall(mallocFunc, {totalSize});
        llvm::Value* typedNewArrayPtr = builder->CreateBitCast(newArrayPtr, llvm::PointerType::getUnqual(elementType));
        
        llvm::Value* newSizeDouble = builder->CreateUIToFP(newSize, elementType);
        builder->CreateStore(newSizeDouble, typedNewArrayPtr);
        
        llvm::Value* newDataPtr = builder->CreateInBoundsGEP(elementType, typedNewArrayPtr,
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1));
        
        llvm::BasicBlock* loopBlock = llvm::BasicBlock::Create(*context, "copy_loop", currentFunction);
        llvm::BasicBlock* loopEndBlock = llvm::BasicBlock::Create(*context, "copy_end", currentFunction);
        
        llvm::AllocaInst* indexVar = builder->CreateAlloca(llvm::Type::getInt64Ty(*context), nullptr, "copy_index");
        builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 0), indexVar);
        
        builder->CreateBr(loopBlock);
        
        builder->SetInsertPoint(loopBlock);
        llvm::Value* currentIndex = builder->CreateLoad(llvm::Type::getInt64Ty(*context), indexVar);
        llvm::Value* condition = builder->CreateICmpULT(currentIndex, currentSizeInt);
        
        llvm::BasicBlock* loopBodyBlock = llvm::BasicBlock::Create(*context, "copy_body", currentFunction);
        builder->CreateCondBr(condition, loopBodyBlock, loopEndBlock);
        
        builder->SetInsertPoint(loopBodyBlock);
        llvm::Value* srcPtr = builder->CreateInBoundsGEP(elementType, arrayPtr, currentIndex);
        llvm::Value* dstPtr = builder->CreateInBoundsGEP(elementType, newDataPtr, currentIndex);
        llvm::Value* element = builder->CreateLoad(elementType, srcPtr);
        builder->CreateStore(element, dstPtr);
        
        llvm::Value* nextIndex = builder->CreateAdd(currentIndex,
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1));
        builder->CreateStore(nextIndex, indexVar);
        builder->CreateBr(loopBlock);
        
        builder->SetInsertPoint(loopEndBlock);
        llvm::Value* lastElementPtr = builder->CreateInBoundsGEP(elementType, newDataPtr, currentSizeInt);
        builder->CreateStore(newValue, lastElementPtr);
        
        valueStack.push(newDataPtr);
        return;
    } else if (node->name == "print") {
        if (node->arguments.empty()) {
            llvm::GlobalVariable* newline = builder->CreateGlobalString("\n");
            std::vector<llvm::Value*> indices = {getInt32(0), getInt32(0)};
            llvm::Value* newlinePtr = builder->CreateInBoundsGEP(
                newline->getValueType(),
                newline,
                indices
            );
            builder->CreateCall(functions["printf"], {newlinePtr});
        } else {
            for (auto& arg : node->arguments) {
                arg->accept(this);
                llvm::Value* value = valueStack.top();
                valueStack.pop();
                
                if (value->getType()->isPointerTy()) {
                    llvm::Value* isString = isStringPointer(value, true);
                    
                    llvm::BasicBlock* stringBlock = llvm::BasicBlock::Create(*context, "print_string", currentFunction);
                    llvm::BasicBlock* numberBlock = llvm::BasicBlock::Create(*context, "print_number", currentFunction);
                    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context, "print_merge", currentFunction);
                    
                    builder->CreateCondBr(isString, stringBlock, numberBlock);
                    
                    builder->SetInsertPoint(stringBlock);
                    builder->CreateCall(functions["printf"], {createFormatString("%s\n"), value});
                    builder->CreateBr(mergeBlock);
                    
                    builder->SetInsertPoint(numberBlock);
                    llvm::Value* doubleVal = unboxValue(value, llvm::Type::getDoubleTy(*context));
                    builder->CreateCall(functions["printf"], {createFormatString("%f\n"), doubleVal});
                    builder->CreateBr(mergeBlock);
                    
                    builder->SetInsertPoint(mergeBlock);
                } else if (value->getType()->isDoubleTy()) {
                    builder->CreateCall(functions["printf"], {createFormatString("%f\n"), value});
                } else if (value->getType()->isIntegerTy()) {
                    builder->CreateCall(functions["printf"], {createFormatString("%d\n"), value});
                }
            }
        }
        valueStack.push(llvm::ConstantInt::get(*context, llvm::APInt(32, 0)));
    } else {
        auto it = functions.find(node->name);
        if (it == functions.end()) {
            throw std::runtime_error("Undefined function: " + node->name);
        }
        
        llvm::Function* func = it->second;
        std::vector<llvm::Value*> args;
        
        for (auto& arg : node->arguments) {
            arg->accept(this);
            llvm::Value* argValue = valueStack.top();
            valueStack.pop();
            
            if (func->getLinkage() == llvm::Function::InternalLinkage) {
                argValue = convertToDouble(argValue);
            }
            
            args.push_back(argValue);
        }
        
        llvm::Value* result = builder->CreateCall(func, args);
        valueStack.push(result);
    }
}

void CodeGenerator::visit(ExpressionStatement* node) {
    node->expression->accept(this);
    if (!valueStack.empty()) {
        valueStack.pop();
    }
}

void CodeGenerator::visit(ArrayLiteral* node) {
    llvm::Type* elementType = llvm::Type::getDoubleTy(*context);
    
    llvm::Function* mallocFunc = module->getFunction("malloc");
    if (!mallocFunc) {
        declareMalloc();
        mallocFunc = module->getFunction("malloc");
    }
    
    // Calculate memory size: (elements + 1 for size metadata) * sizeof(double)
    size_t elementCount = node->elements.size();
    llvm::Value* totalSize = getInt64((elementCount + 1) * 8);
    
    llvm::Value* arrayPtr = builder->CreateCall(mallocFunc, {totalSize});
    llvm::Value* typedArrayPtr = builder->CreateBitCast(arrayPtr, llvm::PointerType::getUnqual(elementType));
    
    llvm::Value* arraySizeDouble = llvm::ConstantFP::get(elementType, static_cast<double>(elementCount));
    builder->CreateStore(arraySizeDouble, typedArrayPtr);
    
    llvm::Value* dataPtr = builder->CreateInBoundsGEP(elementType, typedArrayPtr, getInt64(1));
    
    for (size_t i = 0; i < node->elements.size(); ++i) {
        node->elements[i]->accept(this);
        llvm::Value* value = valueStack.top();
        valueStack.pop();
        
        value = convertToDouble(value);
        
        llvm::Value* elementPtr = builder->CreateInBoundsGEP(elementType, dataPtr, getInt64(i));
        builder->CreateStore(value, elementPtr);
    }
    
    valueStack.push(dataPtr);
}

void CodeGenerator::visit(IndexExpression* node) {
    node->array->accept(this);
    llvm::Value* arrayPtr = valueStack.top();
    valueStack.pop();
    
    node->index->accept(this);
    llvm::Value* index = valueStack.top();
    valueStack.pop();
    
    if (!index->getType()->isIntegerTy()) {
        index = builder->CreateFPToUI(index, llvm::Type::getInt64Ty(*context));
    }
    
    llvm::Type* elementType = llvm::Type::getDoubleTy(*context);
    llvm::Value* elementPtr = builder->CreateInBoundsGEP(elementType, arrayPtr, index);
    llvm::Value* value = builder->CreateLoad(elementType, elementPtr);
    
    valueStack.push(value);
}

void CodeGenerator::visit(IndexAssignmentExpression* node) {
    node->array->accept(this);
    llvm::Value* arrayPtr = valueStack.top();
    valueStack.pop();
    
    node->index->accept(this);
    llvm::Value* index = valueStack.top();
    valueStack.pop();
    
    node->value->accept(this);
    llvm::Value* value = valueStack.top();
    valueStack.pop();
    
    if (!index->getType()->isIntegerTy()) {
        index = builder->CreateFPToUI(index, llvm::Type::getInt64Ty(*context));
    }
    
    llvm::Type* elementType = llvm::Type::getDoubleTy(*context);
    value = convertToDouble(value);
    
    llvm::Value* elementPtr = builder->CreateInBoundsGEP(elementType, arrayPtr, index);
    builder->CreateStore(value, elementPtr);

    valueStack.push(value);
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
            valueStack.pop(); // Discard result
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
        
        llvm::Function* func = builder->GetInsertBlock()->getParent();
        llvm::Type* returnType = func->getReturnType();
        
        if (returnType->isIntegerTy()) {
            if (value->getType()->isDoubleTy()) {
                value = builder->CreateFPToSI(value, llvm::Type::getInt32Ty(*context));
            } else if (value->getType()->isPointerTy()) {
                value = llvm::ConstantInt::get(*context, llvm::APInt(32, 0));
            }
        } else if (returnType->isPointerTy()) {
            if (!value->getType()->isPointerTy()) {
                value = boxValue(value);
            }
        }
        
        builder->CreateRet(value);
    } else {
        llvm::Function* func = builder->GetInsertBlock()->getParent();
        llvm::Type* returnType = func->getReturnType();
        
        if (returnType->isVoidTy()) {
            builder->CreateRetVoid();
        } else if (returnType->isIntegerTy()) {
            builder->CreateRet(llvm::ConstantInt::get(*context, llvm::APInt(32, 0)));
        } else if (returnType->isPointerTy()) {
            builder->CreateRet(llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context)));
        } else {
            builder->CreateRetVoid();
        }
    }
}

void CodeGenerator::visit(FunctionDeclaration* node) {
    auto it = functions.find(node->name);
    llvm::Function* function;
    
    if (it != functions.end()) {
        function = it->second;
    } else {
        std::vector<llvm::Type*> paramTypes;
        for (size_t i = 0; i < node->parameters.size(); i++) {
            paramTypes.push_back(llvm::Type::getDoubleTy(*context));
        }
        
        llvm::FunctionType* funcType = llvm::FunctionType::get(
            llvm::PointerType::getUnqual(*context),
            paramTypes,
            false
        );
        
        function = llvm::Function::Create(
            funcType,
            llvm::Function::InternalLinkage,
            node->name,
            module.get()
        );
        
        functions[node->name] = function;
    }
    
    llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(*context, "entry", function);
    
    llvm::Function* previousFunction = currentFunction;
    llvm::BasicBlock* previousBlock = builder->GetInsertBlock();
    
    currentFunction = function;
    builder->SetInsertPoint(entryBlock);
    
    pushScope();
    
    auto argIt = function->arg_begin();
    for (size_t i = 0; i < node->parameters.size(); i++, ++argIt) {
        llvm::Argument* arg = &*argIt;
        arg->setName(node->parameters[i]);
        
        // Create alloca for parameter and store the argument value (double)
        llvm::AllocaInst* alloca = createEntryBlockAlloca(function, node->parameters[i], 
                                                          llvm::Type::getDoubleTy(*context));
        builder->CreateStore(arg, alloca);
        symbolTables.back()[node->parameters[i]] = alloca;
    }
    
    node->body->accept(this);
     
    if (!builder->GetInsertBlock()->getTerminator()) {
        llvm::Value* nullValue = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context));
        builder->CreateRet(nullValue);
    }

    popScope();
    
    currentFunction = previousFunction;
    if (previousBlock) {
        builder->SetInsertPoint(previousBlock);
    }
    
    std::string error;
    llvm::raw_string_ostream errorStream(error);
    if (llvm::verifyFunction(*function, &errorStream)) {
        std::cerr << "Function verification failed for " << node->name << ": " << error << std::endl;
        function->eraseFromParent();
        functions.erase(node->name);
        throw std::runtime_error("Function generation failed");
    }
}

void CodeGenerator::declareStrlen() {
    if (module->getFunction("strlen")) return;
    
    std::vector<llvm::Type*> params = {
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::Type::getInt64Ty(*context),
        params,
        false
    );
    
    llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "strlen", *module);
}

void CodeGenerator::declareMalloc() {
    if (module->getFunction("malloc")) return;
    
    std::vector<llvm::Type*> params = {
        llvm::Type::getInt64Ty(*context)
    };
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),
        params,
        false
    );
    
    llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "malloc", *module);
}

void CodeGenerator::declareStrcpy() {
    if (module->getFunction("strcpy")) return;
    
    std::vector<llvm::Type*> params = {
        llvm::PointerType::getUnqual(*context),
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),
        params,
        false
    );
    
    llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "strcpy", *module);
}

void CodeGenerator::declareStrcat() {
    if (module->getFunction("strcat")) return;
    
    std::vector<llvm::Type*> params = {
        llvm::PointerType::getUnqual(*context),
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),
        params,
        false
    );
    
    llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "strcat", *module);
}

void CodeGenerator::declareStrstr() {
    if (module->getFunction("strstr")) return;
    
    std::vector<llvm::Type*> params = {
        llvm::PointerType::getUnqual(*context),
        llvm::PointerType::getUnqual(*context)
    };
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),
        params,
        false
    );
    
    llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "strstr", *module);
}

llvm::Value* CodeGenerator::convertToString(llvm::Value* value) {
    if (value->getType()->isPointerTy()) {
        return value;
    }
    
    if (!value->getType()->isDoubleTy()) {
        value = convertToDouble(value);
    }
    llvm::Type* charType = llvm::Type::getInt8Ty(*context);
    llvm::Type* arrayType = llvm::ArrayType::get(charType, 32);
    llvm::AllocaInst* buffer = builder->CreateAlloca(arrayType, nullptr, "str_buffer");
    
    std::vector<llvm::Value*> indices = {
        llvm::ConstantInt::get(*context, llvm::APInt(32, 0)),
        llvm::ConstantInt::get(*context, llvm::APInt(32, 0))
    };
    llvm::Value* bufferPtr = builder->CreateInBoundsGEP(
        arrayType,
        buffer,
        indices
    );
    
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
    
    llvm::Function* snprintfFunc = module->getFunction("snprintf");
    builder->CreateCall(snprintfFunc, {
        bufferPtr,
        llvm::ConstantInt::get(*context, llvm::APInt(32, 32)),
        formatPtr,
        value
    });
    
    return bufferPtr;
}

llvm::Value* CodeGenerator::createStringConcatenation(llvm::Value* left, llvm::Value* right) {
    declareStrlen();
    declareMalloc();
    declareStrcpy();
    declareStrcat();
    if (!left->getType()->isPointerTy()) {
        left = convertToString(left);
    }
    if (!right->getType()->isPointerTy()) {
        right = convertToString(right);
    }
    
    llvm::Function* strlenFunc = module->getFunction("strlen");
    llvm::Value* leftLen = builder->CreateCall(strlenFunc, {left}, "leftlen");
    llvm::Value* rightLen = builder->CreateCall(strlenFunc, {right}, "rightlen");
    
    llvm::Value* totalLen = builder->CreateAdd(leftLen, rightLen, "addlen");
    totalLen = builder->CreateAdd(totalLen, llvm::ConstantInt::get(*context, llvm::APInt(64, 1)), "totallen");
    llvm::Function* mallocFunc = module->getFunction("malloc");
    llvm::Value* resultPtr = builder->CreateCall(mallocFunc, {totalLen}, "result");
    
    llvm::Type* charPtrType = llvm::PointerType::getUnqual(*context);
    resultPtr = builder->CreateBitCast(resultPtr, charPtrType, "resultstr");
    llvm::Function* strcpyFunc = module->getFunction("strcpy");
    builder->CreateCall(strcpyFunc, {resultPtr, left});
    
    llvm::Function* strcatFunc = module->getFunction("strcat");
    builder->CreateCall(strcatFunc, {resultPtr, right});
    
    return resultPtr;
}