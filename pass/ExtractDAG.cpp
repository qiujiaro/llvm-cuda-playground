//===- ExtractDAG.cpp - emit a scheduling DAG from LLVM IR ---------------===//
//
// out-of-tree New-PM plugin (10-Day guide, Day 1-2).
//
// For each function it picks the largest basic block (the scheduling region),
// builds a data-dependency DAG from SSA def-use edges, and writes the mirsched
// JSON DAG format that engine/ consumes.
//
// This is the ONLY part of the project that depends on LLVM headers. The
// dependency-extraction logic (which edges to add) is yours to refine on Day 2-3;
// the JSON printing is boilerplate.
//
// Build + run: see pass/CMakeLists.txt. Emit with:
//   opt -load-pass-plugin=.../MirSchedPass.dylib \
//       -passes='extract-dag<dir=corpus/dag>' -disable-output foo.ll
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

using namespace llvm;

namespace {

std::string jsonEscape(StringRef s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') { out.push_back('\\'); out.push_back(c); }
        else if (c == '\n') out += "\\n";
        else out.push_back(c);
    }
    return out;
}

// Pick the basic block with the most instructions as the scheduling region.
BasicBlock *largestBlock(Function &F) {
    BasicBlock *best = nullptr;
    size_t bestN = 0;
    for (BasicBlock &BB : F) {
        const size_t n = BB.size();
        if (n > bestN) { bestN = n; best = &BB; }
    }
    return best;
}

struct ExtractDAG : PassInfoMixin<ExtractDAG> {
    std::string outDir;

    explicit ExtractDAG(std::string dir) : outDir(std::move(dir)) {}

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
        if (F.isDeclaration()) return PreservedAnalyses::all();
        BasicBlock *BB = largestBlock(F);
        if (!BB || BB->size() < 2) return PreservedAnalyses::all();

        // Assign a contiguous id to each instruction in the region.
        std::unordered_map<const Instruction *, int> id;
        std::vector<const Instruction *> nodes;
        for (Instruction &I : *BB) {
            id[&I] = static_cast<int>(nodes.size());
            nodes.push_back(&I);
        }

        // Data edges: for each instruction, connect operands defined in-region.
        std::string edges;
        bool firstEdge = true;
        auto addEdge = [&](int from, int to, const char *kind) {
            if (!firstEdge) edges += ",\n";
            firstEdge = false;
            edges += "    {\"from\": " + std::to_string(from) + ", \"to\": "
                   + std::to_string(to) + ", \"kind\": \"" + kind + "\"}";
        };
        for (const Instruction *I : nodes) {
            for (const Use &U : I->operands()) {
                if (auto *Def = dyn_cast<Instruction>(U.get())) {
                    auto it = id.find(Def);
                    if (it != id.end()) addEdge(it->second, id[I], "data");
                }
            }
        }

        // Nodes.
        std::string nodesJson;
        bool firstNode = true;
        for (const Instruction *I : nodes) {
            const bool isLoad = isa<LoadInst>(I);
            const bool isStore = isa<StoreInst>(I);
            if (!firstNode) nodesJson += ",\n";
            firstNode = false;
            nodesJson += "    {\"id\": " + std::to_string(id[I])
                       + ", \"opcode\": \"" + jsonEscape(I->getOpcodeName())
                       + "\", \"isLoad\": " + (isLoad ? "true" : "false")
                       + ", \"isStore\": " + (isStore ? "true" : "false") + "}";
        }

        std::string json = "{\n  \"function\": \"" + jsonEscape(F.getName())
            + "\",\n  \"block\": \"" + jsonEscape(BB->getName())
            + "\",\n  \"nodes\": [\n" + nodesJson + "\n  ],\n  \"edges\": [\n"
            + edges + "\n  ]\n}\n";

        if (outDir.empty()) {
            outs() << json;
        } else {
            std::error_code EC;
            sys::fs::create_directories(outDir);
            const std::string path = outDir + "/" + F.getName().str() + ".json";
            raw_fd_ostream os(path, EC);
            if (!EC) os << json;
            else errs() << "[mirsched] cannot write " << path << ": "
                        << EC.message() << "\n";
        }
        return PreservedAnalyses::all();
    }
};

}  // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "mirsched", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    // Accept "extract-dag" and "extract-dag<dir=path>".
                    StringRef n = Name;
                    if (!n.consume_front("extract-dag")) return false;
                    std::string dir;
                    if (n.consume_front("<dir=") && n.consume_back(">"))
                        dir = n.str();
                    else if (!n.empty())
                        return false;
                    FPM.addPass(ExtractDAG(dir));
                    return true;
                });
        }};
}
