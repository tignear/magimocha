cmake_minimum_required (VERSION 3.8)


add_library(magimocha-typer STATIC "magimocha/operation.cpp")
target_link_libraries(magimocha-typer magimocha-ast)

target_include_directories (magimocha-typer PUBLIC magimocha-ast ./)