cmake_minimum_required(VERSION 3.16)

project(engine LANGUAGES CXX)

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
