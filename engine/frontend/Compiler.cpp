#include "frontend/Compiler.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

namespace engine::frontend {

namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

}  // namespace

CompilationResult Compiler::compileToLLVMIR(
    std::string_view source,
    const CompilerOptions& options) const {
    // TODO: Replace the Clang subprocess with Clang's frontend APIs.

    CompilationResult result;
    if (source.empty()) {
        result.diagnostics = "source code is empty";
        return result;
    }

    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto tempDirectory = std::filesystem::temp_directory_path()
        / ("llvm-engine-" + std::to_string(nonce));
    std::filesystem::create_directories(tempDirectory);

    const bool isC = options.language == SourceLanguage::C;
    const auto sourcePath = tempDirectory / (isC ? "input.c" : "input.cpp");
    const auto outputPath = tempDirectory / "output.ll";
    const auto diagnosticsPath = tempDirectory / "diagnostics.txt";

    {
        std::ofstream output(sourcePath);
        output << source;
    }

    const std::string optimizationLevel =
        options.optimizationLevel == "O1" || options.optimizationLevel == "O2"
            || options.optimizationLevel == "O3" || options.optimizationLevel == "Os"
            || options.optimizationLevel == "Oz"
        ? options.optimizationLevel
        : "O0";
    const std::string compiler = isC ? "clang" : "clang++";
    const std::string command = compiler + " -S -emit-llvm -" + optimizationLevel
        + " \"" + sourcePath.string() + "\" -o \"" + outputPath.string()
        + "\" 2>\"" + diagnosticsPath.string() + "\"";

    const int exitCode = std::system(command.c_str());
    result.diagnostics = readFile(diagnosticsPath);
    if (exitCode != 0 || !std::filesystem::exists(outputPath)) {
        std::filesystem::remove_all(tempDirectory);
        return result;
    }

    result.success = true;
    result.llvmIR = readFile(outputPath);
    std::filesystem::remove_all(tempDirectory);
    return result;
}

}  // namespace engine::frontend
