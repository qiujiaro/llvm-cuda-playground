#pragma once

#include <cstddef>
#include <string_view>

namespace engine::middleend {

struct IRStatistics {
    std::size_t functionCount = 0;
    std::size_t basicBlockCount = 0;
    std::size_t instructionCount = 0;
};

class Statistics {
public:
    [[nodiscard]] IRStatistics collect(std::string_view llvmIR) const;
};

}  // namespace engine::middleend
