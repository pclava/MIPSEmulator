cmake_minimum_required(VERSION 4.0)
project(MIPSEmulator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory(MIPSTest)
add_subdirectory(MIPSLib)
add_subdirectory(Tools)
