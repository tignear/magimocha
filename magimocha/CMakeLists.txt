# CMakeList.txt : magimocha の CMake プロジェクト。ソースを含めて、次を定義します:
# プロジェクト専用ロジックはこちらです。
#
cmake_minimum_required (VERSION 3.8)
include(${PROJECT_SOURCE_DIR}/cmake/DownloadProject/DownloadProject.cmake)
download_project(PROJ cppcp
                GIT_REPOSITORY https://github.com/tignear/cppcp.git
                GIT_TAG master
)

add_subdirectory(${cppcp_SOURCE_DIR} ${cppcp_BINARY_DIR})

add_library(magimocha INTERFACE)
target_include_directories (magimocha INTERFACE ./)
target_link_libraries(magimocha INTERFACE cppcp)
