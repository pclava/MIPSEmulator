cmake_minimum_required(VERSION 3.16)
project(MIPSLib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-W -Wall -Werror) #All Warnings, all warnings are errors

file(GLOB_RECURSE LIB_SOURCES public/*.h public/*.cpp public/*.c private/MIPS.cpp)
add_library(MIPSLib ${LIB_SOURCES})
target_include_directories(MIPSLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public) # allows anything linked to MIPSLib to include the headers
target_include_directories(MIPSLib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private)

add_executable(MIPSEmulator private/MIPS.cpp)
add_dependencies(MIPSEmulator MIPSLib)
target_link_libraries(MIPSEmulator MIPSLib)