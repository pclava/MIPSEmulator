cmake_minimum_required(VERSION 3.16)
add_definitions(-D_GNU_SOURCE)
project(MIPSEmulator VERSION 2.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        main
)
FetchContent_Declare(
        SDL_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG        main
)
FetchContent_MakeAvailable(SDL3 SDL_ttf)

function(embed_file target file symbol)
    get_filename_component(abs "${file}" ABSOLUTE)
    set(out "${CMAKE_CURRENT_BINARY_DIR}/${symbol}.cpp")
    add_custom_command(
            OUTPUT "${out}"
            COMMAND ${CMAKE_COMMAND}
            -DINPUT=${abs} -DOUTPUT=${out} -DSYMBOL=${symbol}
            -P ${CMAKE_SOURCE_DIR}/embed.cmake
            DEPENDS "${abs}" "${CMAKE_SOURCE_DIR}/embed.cmake"
            VERBATIM)
    target_sources(${target} PRIVATE "${out}")
endfunction()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory(MIPSLib)
include(CPack)
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(MIPSTest)
endif()
install(TARGETS MIPSEmulator)