#pragma once

#include <string>
#include <string_view>

namespace engine::backend {

struct AssemblyOptions {
    std::string targetTriple;
    std::string cpu;
    std::string features;
};

struct AssemblyResult {
    bool success = false;
    std::string assembly;
    std::string diagnostics;
};

class AssemblyGenerator {
public:
    [[nodiscard]] AssemblyResult generate(
        std::string_view llvmIR,
        const AssemblyOptions& options = {}) const;
};

}  // namespace engine::backend
