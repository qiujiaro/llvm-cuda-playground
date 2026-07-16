#include "middleend/Statistics.h"

#include <cctype>
#include <sstream>
#include <string>

namespace engine::middleend {

IRStatistics Statistics::collect(std::string_view llvmIR) const {
    // TODO: Replace line-based counting with LLVM's Module/Function APIs.
    IRStatistics statistics;
    std::istringstream input{std::string(llvmIR)};
    std::string line;
    bool insideFunction = false;
    bool sawBasicBlockLabel = false;

    while (std::getline(input, line)) {
        const auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos) {
            continue;
        }
        const auto last = line.find_last_not_of(" \t\r");
        const std::string trimmed = line.substr(first, last - first + 1);

        if (trimmed.rfind("define ", 0) == 0) {
            ++statistics.functionCount;
            insideFunction = true;
            sawBasicBlockLabel = false;
            continue;
        }
        if (!insideFunction || trimmed[0] == ';') {
            continue;
        }
        if (trimmed == "}") {
            if (!sawBasicBlockLabel) {
                ++statistics.basicBlockCount;
            }
            insideFunction = false;
        } else if (trimmed.back() == ':') {
            ++statistics.basicBlockCount;
            sawBasicBlockLabel = true;
        } else {
            ++statistics.instructionCount;
        }
    }

    return statistics;
}

}  // namespace engine::middleend
