#include "middleend/Analyzer.h"

namespace engine::middleend {

AnalysisResult Analyzer::analyze(std::string_view llvmIR) const {
    // TODO: Replace these text checks with LLVM analysis passes.
    AnalysisResult result;
    if (llvmIR.empty()) {
        result.diagnostics = "LLVM IR is empty";
        return result;
    }

    if (llvmIR.find("define ") == std::string_view::npos) {
        result.findings.push_back({
            "structure",
            "no function definition was found",
            "",
        });
    }
    if (llvmIR.find("ret ") == std::string_view::npos) {
        result.findings.push_back({
            "control-flow",
            "no return instruction was found",
            "",
        });
    }

    result.success = true;
    return result;
}

}  // namespace engine::middleend
