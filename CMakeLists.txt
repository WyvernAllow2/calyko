cmake_minimum_required(VERSION 3.10)
project(calyko VERSION 0.1.0 LANGUAGES C)

add_executable(${PROJECT_NAME}
    src/main.c
)

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /permissive-
        /sdl
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -std=c99
        -Wall
        -Wextra
        -Wpedantic
        -Wsign-conversion
        -Wshadow
        -Wstrict-prototypes
        -Wundef
        -Wpointer-arith
        -Wcast-align
        -Wmissing-prototypes
    )
endif()

find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
)

set(SHADERS
    shaders/pathtracer.comp
)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

set(COMPILED_SHADERS "")
foreach(SHADER ${SHADERS})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/shaders/${FILE_NAME}.spv")

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND Vulkan::glslc "${CMAKE_SOURCE_DIR}/${SHADER}" -o ${OUTPUT_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${FILE_NAME}${FILE_EXT} -> ${OUTPUT_FILE}"
        VERBATIM
    )

    list(APPEND COMPILED_SHADERS ${OUTPUT_FILE})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})
add_dependencies(${PROJECT_NAME} Shaders)