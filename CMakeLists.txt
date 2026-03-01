cmake_minimum_required(VERSION 3.16)
add_definitions(-D_GNU_SOURCE)
project(MIPSEmulator VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory(MIPSLib)
include(CPack)
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(MIPSTest)
endif()
install(TARGETS MIPSEmulator DESTINATION bin)