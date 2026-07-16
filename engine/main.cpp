#include "backend/AssemblyGenerator.h"
#include "backend/PTXGenerator.h"
#include "frontend/Compiler.h"
#include "middleend/Analyzer.h"
#include "middleend/Optimizer.h"
#include "middleend/Statistics.h"

#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

namespace {

std::string jsonEscape(std::string_view value) {
    std::ostringstream output;
    for (const unsigned char character : value) {
        switch (character) {
            case '\\': output << "\\\\"; break;
            case '"': output << "\\\""; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (character >= 0x20) {
                    output << character;
                }
        }
    }
    return output.str();
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string source{
        std::istreambuf_iterator<char>(std::cin),
        std::istreambuf_iterator<char>(),
    };
    if (source.empty()) {
        source = "int main() { return 0; }";
    }

    engine::frontend::CompilerOptions compilerOptions;
    if (argc > 1) {
        compilerOptions.optimizationLevel = argv[1];
    }
    if (argc > 3 && std::string_view(argv[3]) == "c") {
        compilerOptions.language = engine::frontend::SourceLanguage::C;
    }

    engine::frontend::Compiler compiler;
    const auto compilation = compiler.compileToLLVMIR(source, compilerOptions);

    engine::middleend::Optimizer optimizer;
    const auto optimization = optimizer.optimize(compilation.llvmIR);

    engine::middleend::Analyzer analyzer;
    const auto analysis = analyzer.analyze(optimization.optimizedIR);

    engine::middleend::Statistics statistics;
    const auto irStatistics = statistics.collect(optimization.optimizedIR);

    engine::backend::PTXGenerator ptxGenerator;
    const auto ptx = ptxGenerator.generate(optimization.optimizedIR);

    engine::backend::AssemblyGenerator assemblyGenerator;
    engine::backend::AssemblyOptions assemblyOptions;
    if (argc > 2) {
        assemblyOptions.targetTriple = argv[2];
    }
    const auto assembly = assemblyGenerator.generate(
        optimization.optimizedIR,
        assemblyOptions);

    const bool success = compilation.success && optimization.success
        && analysis.success && ptx.success && assembly.success;
    const std::string diagnostics = compilation.diagnostics
        + optimization.diagnostics + analysis.diagnostics
        + ptx.diagnostics + assembly.diagnostics;

    std::cout << "{"
              << "\"success\":" << (success ? "true" : "false") << ','
              << "\"llvm_ir\":\"" << jsonEscape(compilation.llvmIR) << "\","
              << "\"optimized_ir\":\"" << jsonEscape(optimization.optimizedIR) << "\","
              << "\"ptx\":\"" << jsonEscape(ptx.ptx) << "\","
              << "\"assembly\":\"" << jsonEscape(assembly.assembly) << "\","
              << "\"diagnostics\":\"" << jsonEscape(diagnostics) << "\","
              << "\"function_count\":" << irStatistics.functionCount << ','
              << "\"basic_block_count\":" << irStatistics.basicBlockCount << ','
              << "\"instruction_count\":" << irStatistics.instructionCount << ','
              << "\"finding_count\":" << analysis.findings.size()
              << "}\n";

    return 0;
}
