#include "backend/AssemblyGenerator.h"

#include <sstream>

namespace engine::backend {

AssemblyResult AssemblyGenerator::generate(
    std::string_view llvmIR,
    const AssemblyOptions& options) const {
    // TODO: Replace this assembly skeleton with LLVM's native target backend.
    AssemblyResult result;
    if (llvmIR.empty()) {
        result.diagnostics = "LLVM IR is empty";
        return result;
    }

    std::ostringstream assembly;
    assembly << "# target: "
             << (options.targetTriple.empty() ? "host" : options.targetTriple)
             << "\n"
             << ".text\n"
             << ".globl main\n"
             << "main:\n"
             << "    ret\n";

    result.success = true;
    result.assembly = assembly.str();
    return result;
}

}  // namespace engine::backend
