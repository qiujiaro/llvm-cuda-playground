#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace engine::frontend {

enum class SourceLanguage {
    C,
    Cpp,
    CUDA,
};

struct CompilerOptions {
    SourceLanguage language = SourceLanguage::Cpp;
    std::string optimizationLevel = "O0";
    std::vector<std::string> extraArguments;
};

struct CompilationResult {
    bool success = false;
    std::string llvmIR;
    std::string diagnostics;
};

class Compiler {
public:
    [[nodiscard]] CompilationResult compileToLLVMIR(
        std::string_view source,
        const CompilerOptions& options = {}) const;
};

}  // namespace engine::frontend
