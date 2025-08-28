cmake_minimum_required(VERSION 3.10)
project(stb_image_write)

add_library(${PROJECT_NAME}
    include/stb_image_write.h
    src/stb_image_write.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
    include/
)
