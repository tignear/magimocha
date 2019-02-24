cmake_minimum_required (VERSION 3.8)

#llvm 
set(LLVM_DIR "C:\\Users\\tignear\\CMakeBuilds\\9a4487f8-4285-ec3c-ab94-794337022481\\install\\x64-Release\\lib\\cmake\\llvm")
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)


add_executable(magimocha-llvm "magimocha/llvm/codegen.cpp")

target_link_libraries(magimocha-llvm magimocha-ast ${llvm_libs})
