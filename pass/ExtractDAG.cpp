//===- ExtractDAG.cpp - mirsched out-of-tree pass (Day 1) ----------------===//
//
// Day-1 version: a New Pass Manager plugin that just prints function names.
// Day 2 extends this to walk the largest basic block, build def-use edges, and
// emit the mirsched DAG JSON.
//
// Build:
//   cmake -S pass -B pass/build -G Ninja \
//         -DLLVM_DIR=$(brew --prefix llvm)/lib/cmake/llvm
//   cmake --build pass/build
//
// Run (must use the SAME LLVM's opt; .dylib on macOS, .so on Linux):
//   $(brew --prefix llvm)/bin/opt \
//       -load-pass-plugin=pass/build/MirSchedPass.dylib \
//       -passes=extract-dag -disable-output input.ll
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"

// PassPlugin.h moved from llvm/Passes/ to llvm/Plugins/ in LLVM 22.
#if __has_include("llvm/Plugins/PassPlugin.h")
#include "llvm/Plugins/PassPlugin.h"
#else
#include "llvm/Passes/PassPlugin.h"
#endif

using namespace llvm;

namespace {

struct ExtractDAG : PassInfoMixin<ExtractDAG> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    errs() << "[mirsched] function: " << F.getName() << "\n";
    return PreservedAnalyses::all();
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "mirsched", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "extract-dag") {
                    FPM.addPass(ExtractDAG());
                    return true;
                  }
                  return false;
                });
          }};
}
