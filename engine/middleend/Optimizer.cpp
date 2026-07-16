#include "middleend/Optimizer.h"

namespace engine::middleend {

OptimizationResult Optimizer::optimize(
    std::string_view llvmIR,
    const OptimizerOptions& options) const {
    // TODO: Replace this pass-through implementation with LLVM PassBuilder.
    (void)options;

    OptimizationResult result;
    if (llvmIR.empty()) {
        result.diagnostics = "LLVM IR is empty";
        return result;
    }

    result.success = true;
    result.optimizedIR = std::string(llvmIR);
    return result;
}

}  // namespace engine::middleend
