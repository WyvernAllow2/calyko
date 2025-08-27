cmake_minimum_required(VERSION 3.10)
project(VulkanMemoryAllocator)

add_library(${PROJECT_NAME}
    include/vk_mem_alloc.h
    src/vk_mem_alloc.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    include/
)

find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
)