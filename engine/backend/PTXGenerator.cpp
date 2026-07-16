#include "backend/PTXGenerator.h"

#include <sstream>

namespace engine::backend {

PTXResult PTXGenerator::generate(
    std::string_view llvmIR,
    const PTXOptions& options) const {
    // TODO: Replace this PTX skeleton with LLVM's NVPTX target backend.
    PTXResult result;
    if (llvmIR.empty()) {
        result.diagnostics = "LLVM IR is empty";
        return result;
    }

    std::ostringstream ptx;
    ptx << ".version 7.0\n"
        << ".target " << options.gpuArchitecture << "\n"
        << ".address_size 64\n\n"
        << ".visible .entry main()\n"
        << "{\n"
        << "    ret;\n"
        << "}\n";

    result.success = true;
    result.ptx = ptx.str();
    return result;
}

}  // namespace engine::backend
