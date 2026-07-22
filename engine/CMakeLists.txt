cmake_minimum_required(VERSION 3.16)

project(engine LANGUAGES CXX)

# ---------------------------------------------------------------------------
# mirsched core: the 10-Day guide's project. Pure C++, NO LLVM dependency, so
# it builds and tests anywhere (including CI without a full LLVM install).
# ---------------------------------------------------------------------------
add_library(mirsched_core STATIC
    src/Json.cpp
    src/Dag.cpp
    src/CostModel.cpp
    src/ListScheduler.cpp
    src/LinearScan.cpp
)
target_include_directories(mirsched_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(mirsched_core PUBLIC cxx_std_17)

add_executable(mirsched tools/mirsched-cli.cpp)
target_link_libraries(mirsched PRIVATE mirsched_core)

enable_testing()
add_executable(mirsched_tests
    ../tests/test_main.cpp
    ../tests/test_scheduler.cpp
    ../tests/test_linearscan.cpp
)
target_link_libraries(mirsched_tests PRIVATE mirsched_core)
target_compile_definitions(mirsched_tests PRIVATE
    MIRSCHED_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../tests/golden")
add_test(NAME mirsched_tests COMMAND mirsched_tests)

# ---------------------------------------------------------------------------
# Legacy web-demo engine (frontend/backend playground). Kept as the optional
# dashboard product described in docs/ARCHITECTURE.md. Not part of the core.
# ---------------------------------------------------------------------------
add_library(engine STATIC
    frontend/Compiler.cpp
    middleend/Optimizer.cpp
    middleend/Statistics.cpp
    middleend/Analyzer.cpp
    backend/PTXGenerator.cpp
    backend/AssemblyGenerator.cpp
)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(engine PUBLIC cxx_std_17)

add_executable(engine_cli main.cpp)
target_link_libraries(engine_cli PRIVATE engine)
target_compile_features(engine_cli PRIVATE cxx_std_17)
set_target_properties(engine_cli PROPERTIES OUTPUT_NAME llvm-engine)
