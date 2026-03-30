cmake_minimum_required(VERSION 3.26)
project(Checkers)

set(CMAKE_CXX_STANDARD 20)

set(BUILD_SHARED_LIBS FALSE)

include(FetchContent)
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.x
)
FetchContent_MakeAvailable(SFML)

add_executable(Checkers src/main.cpp src/Board.cpp
        src/AI.cpp
        src/AI.h
)

target_compile_definitions(Checkers PRIVATE SFML_STATIC)

target_link_libraries(Checkers
        sfml-graphics
        sfml-audio
        sfml-window
        sfml-system
        winmm
        opengl32
        gdi32
)

target_link_options(Checkers PRIVATE
        -static-libgcc
        -static-libstdc++
        -static
)
