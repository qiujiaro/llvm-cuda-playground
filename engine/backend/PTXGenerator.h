#pragma once

#include <string>
#include <string_view>

namespace engine::backend {

struct PTXOptions {
    std::string gpuArchitecture = "sm_80";
    std::string ptxVersion;
};

struct PTXResult {
    bool success = false;
    std::string ptx;
    std::string diagnostics;
};

class PTXGenerator {
public:
    [[nodiscard]] PTXResult generate(
        std::string_view llvmIR,
        const PTXOptions& options = {}) const;
};

}  // namespace engine::backend
