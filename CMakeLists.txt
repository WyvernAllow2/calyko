cmake_minimum_required(VERSION 3.10)
project(calyko VERSION 0.1.0 LANGUAGES C)

add_executable(${PROJECT_NAME}
    src/arena.c
    src/arena.h
    src/main.c
)

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -std=c99 -pedantic -Wall -Wextra)
endif()