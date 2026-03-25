cmake_minimum_required(VERSION 3.16)
add_definitions(-D_GNU_SOURCE)
project(MIPSEmulator VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        main # Or a specific release tag like '3.2.0'
)
FetchContent_MakeAvailable(SDL3)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory(MIPSLib)
include(CPack)
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(MIPSTest)
endif()
install(TARGETS MIPSEmulator)