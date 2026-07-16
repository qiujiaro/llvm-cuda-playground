#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace engine::middleend {

struct AnalysisFinding {
    std::string category;
    std::string message;
    std::string functionName;
};

struct AnalysisResult {
    bool success = false;
    std::vector<AnalysisFinding> findings;
    std::string diagnostics;
};

class Analyzer {
public:
    [[nodiscard]] AnalysisResult analyze(std::string_view llvmIR) const;
};

}  // namespace engine::middleend
