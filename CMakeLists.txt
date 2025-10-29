cmake_minimum_required(VERSION 3.20.0)
project(picayune VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_executable(${PROJECT_NAME} src/main.cpp src/parser.cpp src/span.cpp src/tokenizer.cpp src/typechecker.cpp src/codegen.cpp src/extra/printer.cpp)

llvm_map_components_to_libnames(llvm_libs -21)

target_link_libraries(${PROJECT_NAME} ${llvm_libs})

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -Wpedantic)
