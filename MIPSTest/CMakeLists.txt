cmake_minimum_required(VERSION 3.16)
project(MIPSTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-W -Wall -Werror) #All Warnings, all warnings are errors

include(FetchContent)
FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(TEST_SOURCES src/memory_tests.cpp src/register_tests.cpp src/processor_tests.cpp src/instruction_tests.cpp
        src/syscall_tests.cpp)

add_executable(MIPS_TEST ${TEST_SOURCES})
add_dependencies(MIPS_TEST MIPSLib)
target_link_libraries(MIPS_TEST GTest::gtest_main)
target_link_libraries(MIPS_TEST MIPSLib)

include(GoogleTest)
gtest_discover_tests(MIPS_TEST)