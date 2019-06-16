cmake_minimum_required (VERSION 3.10)
include(${PROJECT_SOURCE_DIR}/cmake/DownloadProject/DownloadProject.cmake)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

download_project(PROJ googletest
                GIT_REPOSITORY https://github.com/google/googletest.git
                GIT_TAG master
                UPDATE_DISCONNECTED 1
)
#add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})

add_executable(magimocha-test "magimocha-test.cpp" "typing-test.cpp")


target_link_libraries(magimocha-test gtest_main magimocha-ast magimocha-typer)
include_directories(${PROJECT_SOURCE_DIR}/src)

include(GoogleTest)

add_test(NAME magimocha-test COMMAND magimocha-test)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stack:2700000")	
