# CMakeList.txt : 最上位の CMake プロジェクト ファイル。グローバル構成を行います
# また、サブプロジェクトをここに含めます。
#
cmake_minimum_required (VERSION 3.8)
enable_testing()
set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_CXX_EXTENSIONS OFF) 


project ("magimocha-distribution")

# サブプロジェクトを含めます。
add_subdirectory ("magimocha-ast")
add_subdirectory ("magimocha-llvm2")
add_subdirectory ("magimocha-typer")

add_subdirectory ("magimocha-test")

