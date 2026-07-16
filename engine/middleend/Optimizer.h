#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace engine::middleend {

struct OptimizerOptions {
    std::string optimizationLevel = "O2";
    std::vector<std::string> passes;
};

struct OptimizationResult {
    bool success = false;
    std::string optimizedIR;
    std::string diagnostics;
};

class Optimizer {
public:
    [[nodiscard]] OptimizationResult optimize(
        std::string_view llvmIR,
        const OptimizerOptions& options = {}) const;
};

}  // namespace engine::middleend
