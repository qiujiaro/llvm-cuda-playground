cmake_minimum_required(VERSION 3.20)
project(mirsched_pass LANGUAGES CXX)

# Build against an existing LLVM (e.g. Homebrew: brew install llvm).
#   cmake -S pass -B pass/build -G Ninja \
#         -DLLVM_DIR=$(brew --prefix llvm)/lib/cmake/llvm
#   cmake --build pass/build
#
# On macOS the plugin is MirSchedPass.dylib; load it with the SAME LLVM's opt:
#   $(brew --prefix llvm)/bin/opt -load-pass-plugin=pass/build/MirSchedPass.dylib \
#       -passes=extract-dag -disable-output input.ll
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Using LLVMConfig from ${LLVM_DIR} (LLVM ${LLVM_PACKAGE_VERSION})")

list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
include(AddLLVM)

add_llvm_pass_plugin(MirSchedPass ExtractDAG.cpp)
