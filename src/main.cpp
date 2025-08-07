#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/codegen.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string getBaseName(const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);
    
    size_t lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) {
        return filename.substr(0, lastDot);
    }
    return filename;
}

std::string getOutputExecutable(const std::string& baseName) {
#ifdef _WIN32
    return baseName + ".exe";
#else
    return baseName;
#endif
}

int runCommand(const std::string& command) {
    std::cout << "Running: " << command << std::endl;
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Command failed with exit code: " << result << std::endl;
    }
    return result;
}

void printUsage(const std::string& programName) {
    std::cout << "Usage: " << programName << " <input.tw> [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -o <output>    Specify output executable name" << std::endl;
    std::cout << "  --emit-ir      Output LLVM IR only" << std::endl;
    std::cout << "  --emit-asm     Output assembly only" << std::endl;
    std::cout << "  --emit-obj     Output object file only" << std::endl;
    std::cout << "  --verbose      Enable verbose output" << std::endl;
    std::cout << "  --version      Show version information" << std::endl;
    std::cout << "  --help         Show this help message" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string inputFile;
    std::string outputFile;
    bool emitIR = false;
    bool emitAsm = false;
    bool emitObj = false;
    bool verbose = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg == "--emit-ir") {
            emitIR = true;
        } else if (arg == "--emit-asm") {
            emitAsm = true;
        } else if (arg == "--emit-obj") {
            emitObj = true;
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "Twine Compiler v1.0.0" << std::endl;
            std::cout << "Built on " << __DATE__ << " " << __TIME__ << std::endl;
            std::cout << "Copyright (c) 2025 Cooper Ross" << std::endl;
            return 0;
        } else if (arg[0] != '-') {
            inputFile = arg;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // Check if input file has .tw extension
    if (inputFile.length() < 3 || inputFile.substr(inputFile.length() - 3) != ".tw") {
        std::cerr << "Error: Input file must have .tw extension" << std::endl;
        return 1;
    }
    
    try {
        // Read source file
        if (verbose) std::cout << "Reading source file: " << inputFile << std::endl;
        std::string source = readFile(inputFile);
        
        // Lexical analysis
        if (verbose) std::cout << "Performing lexical analysis..." << std::endl;
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();
        
        if (verbose) {
            std::cout << "Found " << tokens.size() << " tokens" << std::endl;
        }
        
        // Parsing
        if (verbose) std::cout << "Parsing..." << std::endl;
        Parser parser(tokens);
        std::unique_ptr<Program> ast = parser.parse();
        
        if (!ast) {
            std::cerr << "Parsing failed" << std::endl;
            return 1;
        }
        
        // Code generation
        if (verbose) std::cout << "Generating LLVM IR..." << std::endl;
        std::string baseName = getBaseName(inputFile);
        CodeGenerator codegen(baseName);
        
        if (!codegen.generate(ast.get())) {
            std::cerr << "Code generation failed" << std::endl;
            return 1;
        }
        
        // Write LLVM IR to file
        std::string irFile = baseName + ".ll";
        std::string originalIrFile = irFile;  // Keep track of original IR file
        if (verbose) std::cout << "Writing LLVM IR to: " << irFile << std::endl;
        if (!codegen.writeIRToFile(irFile)) {
            std::cerr << "Failed to write IR file" << std::endl;
            return 1;
        }
        
        if (emitIR) {
            std::cout << "LLVM IR written to: " << irFile << std::endl;
            return 0;
        }
        
        // Optimize with opt (optional, but recommended)
        std::string optFile = baseName + "_opt.ll";
        std::string optCmd = "opt -O2 -S " + irFile + " -o " + optFile;
        if (runCommand(optCmd) == 0) {
            irFile = optFile;  // Use optimized version
            if (verbose) std::cout << "Optimization completed" << std::endl;
        } else {
            if (verbose) std::cout << "Optimization skipped (opt not available or failed)" << std::endl;
        }
        
        // Generate assembly with llc
        std::string asmFile = baseName + ".s";
        std::string llcCmd = "llc -filetype=asm " + irFile + " -o " + asmFile;
        
        if (verbose) std::cout << "Generating assembly..." << std::endl;
        if (runCommand(llcCmd) != 0) {
            std::cerr << "Assembly generation failed" << std::endl;
            return 1;
        }
        
        if (emitAsm) {
            std::cout << "Assembly written to: " << asmFile << std::endl;
            return 0;
        }
        
        // Generate object file with llc
        std::string objFile = baseName + ".o";
        llcCmd = "llc -filetype=obj " + irFile + " -o " + objFile;
        
        if (verbose) std::cout << "Generating object file..." << std::endl;
        if (runCommand(llcCmd) != 0) {
            std::cerr << "Object file generation failed" << std::endl;
            return 1;
        }
        
        if (emitObj) {
            std::cout << "Object file written to: " << objFile << std::endl;
            return 0;
        }
        
        // Link to create executable
        if (outputFile.empty()) {
            outputFile = getOutputExecutable(baseName);
        }
        
        std::string linkCmd;
#ifdef _WIN32
        // On Windows with MinGW
        linkCmd = "gcc " + objFile + " -o " + outputFile + " -lm";
#else
        // On Unix-like systems
        linkCmd = "gcc " + objFile + " -o " + outputFile + " -lm";
#endif
        
        if (verbose) std::cout << "Linking executable..." << std::endl;
        if (runCommand(linkCmd) != 0) {
            // Try with g++ if gcc fails
            linkCmd = "g++ " + objFile + " -o " + outputFile + " -lm";
            if (runCommand(linkCmd) != 0) {
                // Try with ld directly as last resort
                linkCmd = "ld " + objFile + " -o " + outputFile;
#ifdef __linux__
                linkCmd += " /lib64/ld-linux-x86-64.so.2 -lc -dynamic-linker /lib64/ld-linux-x86-64.so.2";
#endif
                if (runCommand(linkCmd) != 0) {
                    std::cerr << "Linking failed" << std::endl;
                    return 1;
                }
            }
        }
        
        std::cout << "Compilation successful!" << std::endl;
        std::cout << "Executable: " << outputFile << std::endl;
        
        // Clean up intermediate files (unless verbose mode)
        if (!verbose && !emitIR && !emitAsm && !emitObj) {
            std::remove(originalIrFile.c_str());  // Remove original .ll file
            if (originalIrFile != irFile) {
                std::remove(irFile.c_str());  // Remove optimized .ll file if different
            }
            std::remove(asmFile.c_str());
            std::remove(objFile.c_str());
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}