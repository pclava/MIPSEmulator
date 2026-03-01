cmake_minimum_required(VERSION 3.16)
project(MIPSLib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-W -Wall -Werror) #All Warnings, all warnings are errors

set(LIB_SOURCES public/Memory.h public/InstructionSet.h public/InstructionSet.cpp public/Processor.h public/Register.h public/utils.h private/MIPS.cpp
        public/Memory.cpp
        public/Register.cpp
        public/Processor.cpp
        public/Coprocessor0.cpp
        public/Syscalls.cpp
        public/Syscalls.h)
add_library(MIPSLib ${LIB_SOURCES})
target_include_directories(MIPSLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public) # allows anything linked to MIPSLib to include the headers
target_include_directories(MIPSLib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private)

add_executable(MIPSEmulator private/MIPS.cpp)
add_dependencies(MIPSEmulator MIPSLib)
target_link_libraries(MIPSEmulator MIPSLib)