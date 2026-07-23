cmake_minimum_required(VERSION 3.20)
# C must be enabled: LLVM's AddLLVM/HandleLLVMOptions run C compiler+linker checks.
project(mirsched_pass LANGUAGES C CXX)

# LLVM 16+ headers require C++17; must be set before the targets are created.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build against an existing LLVM (e.g. Homebrew: brew install llvm).
#   cmake -S pass -B pass/build -G Ninja \
#         -DLLVM_DIR=$(brew --prefix llvm)/lib/cmake/llvm
#   cmake --build pass/build
#
# Load with the SAME LLVM's opt (.dylib on macOS, .so on Linux):
#   $(brew --prefix llvm)/bin/opt \
#       -load-pass-plugin=pass/build/MirSchedPass.dylib \
#       -passes=extract-dag -disable-output input.ll
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Using LLVMConfig from ${LLVM_DIR} (LLVM ${LLVM_PACKAGE_VERSION})")

list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
include(AddLLVM)
include(HandleLLVMOptions)   # sets C++ standard, -fno-rtti, warning flags to match LLVM

# Without these the plugin cannot find <llvm/...> headers.
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND "${LLVM_DEFINITIONS}")
add_definitions(${LLVM_DEFINITIONS_LIST})
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

add_llvm_pass_plugin(MirSchedPass ExtractDAG.cpp)
