#include <assert.h>
#include <stb_image_write.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#include "device.h"
#include "pipeline.h"
#include "utils.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                      VkDebugUtilsMessageTypeFlagsEXT messageType,
                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
    (void)messageType;
    (void)pUserData;
    if (!pCallbackData->pMessage) {
        return VK_FALSE;
    }

    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        fprintf(stderr, "Verbose: %s\n", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        fprintf(stderr, "Info: %s\n", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        fprintf(stderr, "Warning: %s\n", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        fprintf(stderr, "Error: %s\n", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
        break;
    }

    return VK_FALSE;
}

static bool has_extension(const VkExtensionProperties *extensions, uint32_t extension_count,
                          const char *extension_name) {
    for (uint32_t i = 0; i < extension_count; i++) {
        if (strcmp(extensions[i].extensionName, extension_name) == 0) {
            return true;
        }
    }
    return false;
}

static bool has_required_extensions(const char **required, uint32_t required_count) {
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
    VkExtensionProperties *extensions = malloc(sizeof(*extensions) * extension_count);
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extensions);

    for (uint32_t i = 0; i < required_count; i++) {
        if (!has_extension(extensions, extension_count, required[i])) {
            fprintf(stderr, "Missing extension: %s\n", required[i]);
            free(extensions);
            return false;
        }
    }

    free(extensions);
    return true;
}

static bool has_layer(const VkLayerProperties *layers, uint32_t layer_count,
                      const char *layer_name) {
    for (uint32_t i = 0; i < layer_count; i++) {
        if (strcmp(layers[i].layerName, layer_name) == 0) {
            return true;
        }
    }
    return false;
}

static bool has_required_layers(const char **required, uint32_t required_count) {
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    VkLayerProperties *layers = malloc(sizeof(*layers) * layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, layers);

    for (uint32_t i = 0; i < required_count; i++) {
        if (!has_layer(layers, layer_count, required[i])) {
            fprintf(stderr, "Missing layer: %s\n", required[i]);
            free(layers);
            return false;
        }
    }

    free(layers);
    return true;
}

static VkInstance create_instance(const VkDebugUtilsMessengerCreateInfoEXT *debug_info) {
    assert(debug_info);

    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Calyko",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    const char *extensions[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };

    const char *layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    if (!has_required_extensions(extensions, ARRAY_LEN(extensions))) {
        fprintf(stderr, "Missing required extensions\n");
        return NULL;
    }

    if (!has_required_layers(layers, ARRAY_LEN(layers))) {
        fprintf(stderr, "Missing required layers\n");
        return NULL;
    }

    const VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .ppEnabledExtensionNames = extensions,
        .enabledExtensionCount = ARRAY_LEN(extensions),
        .ppEnabledLayerNames = layers,
        .enabledLayerCount = ARRAY_LEN(layers),
        .pNext = debug_info,
    };

    VkInstance instance = NULL;
    VkResult result = vkCreateInstance(&instance_info, NULL, &instance);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateInstance() failed: %s\n", string_VkResult(result));
        return NULL;
    }

    return instance;
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator,
                                             VkDebugUtilsMessengerEXT *pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
                                                                  "vkCreateDebugUtilsMessengerEXT");
    if (func) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                          VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks *pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func) {
        func(instance, debugMessenger, pAllocator);
    }
}

static VkDebugUtilsMessengerEXT create_debug_messenger(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *debug_info) {
    VkDebugUtilsMessengerEXT messenger;
    VkResult result = CreateDebugUtilsMessengerEXT(instance, debug_info, NULL, &messenger);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDebugUtilsMessengerEXT() failed: %s\n", string_VkResult(result));
        return NULL;
    }

    return messenger;
}

static uint32_t *read_spirv_file(const char *filename, uint32_t *word_count) {
    *word_count = 0;

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror(filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(file);
    rewind(file);

    if (file_size % 4 != 0) {
        fprintf(stderr, "SPIR-V file size is not a multiple of 4: %d bytes\n", file_size);
        fclose(file);
        return NULL;
    }

    size_t num_words = file_size / 4;
    uint32_t *words = malloc(num_words * sizeof(uint32_t));
    if (!words) {
        perror("malloc failed");
        fclose(file);
        return NULL;
    }

    if (fread(words, sizeof(uint32_t), num_words, file) != num_words) {
        perror("fread failed");
        free(words);
        fclose(file);
        return NULL;
    }

    fclose(file);
    *word_count = num_words;
    return words;
}

static VkShaderModule load_shader_module(VkDevice device, const char *filename) {
    uint32_t word_count;
    uint32_t *code = read_spirv_file(filename, &word_count);
    if (!code) {
        fprintf(stderr, "read_file_bytes failed\n");
        return VK_NULL_HANDLE;
    }

    const VkShaderModuleCreateInfo shader_module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pCode = code,
        .codeSize = word_count * sizeof(uint32_t),
    };

    VkShaderModule shader_module;
    VkResult result = vkCreateShaderModule(device, &shader_module_info, NULL, &shader_module);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateShaderModule failed: %s\n", string_VkResult(result));
        free(code);
        return VK_NULL_HANDLE;
    }

    free(code);
    return shader_module;
}

static VkDescriptorPool create_descriptor_pool(VkDevice device) {
    const VkDescriptorPoolCreateInfo descriptor_pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .pPoolSizes =
            &(VkDescriptorPoolSize){
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1,
            },
        .poolSizeCount = 1,
    };

    VkDescriptorPool descriptor_pool;
    VkResult result = vkCreateDescriptorPool(device, &descriptor_pool_info, NULL, &descriptor_pool);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDescriptorPool() failed: %s\n", string_VkResult(result));
        return VK_NULL_HANDLE;
    }

    return descriptor_pool;
}

static VkDescriptorSet create_descriptor_set(VkDevice device, VkDescriptorPool descriptor_pool,
                                             VkDescriptorSetLayout layout) {
    const VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet descriptor_set;
    VkResult result = vkAllocateDescriptorSets(device, &alloc_info, &descriptor_set);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkAllocateDescriptorSets failed: %s\n", string_VkResult(result));
        return VK_NULL_HANDLE;
    }

    return descriptor_set;
}

static VmaAllocator create_vma_allocator(VkInstance instance, VkDevice device,
                                         const Physical_Device_Info *info) {
    const VmaAllocatorCreateInfo vma_allocator_info = {
        .vulkanApiVersion = VK_API_VERSION_1_0,
        .physicalDevice = info->physical_device,
        .device = device,
        .instance = instance,
    };

    VmaAllocator allocator;
    VkResult result = vmaCreateAllocator(&vma_allocator_info, &allocator);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vmaCreateAllocator() failed: %s\n", string_VkResult(result));
        return NULL;
    }

    return allocator;
}

static VkImage create_compute_image(VmaAllocator allocator, uint32_t width, uint32_t height,
                                    VkFormat format, VmaAllocation *allocation) {
    const VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent =
            (VkExtent3D){
                .width = width,
                .height = height,
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    const VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    VkImage image;
    VkResult result = vmaCreateImage(allocator, &image_info, &alloc_info, &image, allocation, NULL);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vmaCreateImage() failed: %s\n", string_VkResult(result));
        return VK_NULL_HANDLE;
    }

    return image;
}

static VkImageView create_compute_image_view(VkDevice device, VkImage image, VkFormat format) {
    const VkImageViewCreateInfo image_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange =
            (VkImageSubresourceRange){
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    VkImageView view;
    VkResult result = vkCreateImageView(device, &image_view_info, NULL, &view);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateImageView() failed: %s\n", string_VkResult(result));
        return VK_NULL_HANDLE;
    }

    return view;
}

static VkCommandPool create_command_pool(VkDevice device, const Physical_Device_Info *info) {
    const VkCommandPoolCreateInfo command_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = info->compute_family_index,
    };

    VkCommandPool command_pool;
    VkResult result = vkCreateCommandPool(device, &command_pool_info, NULL, &command_pool);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateCommandPool() failed: %s\n", string_VkResult(result));
        return VK_NULL_HANDLE;
    }

    return command_pool;
}

static VkCommandBuffer create_command_buffer(VkDevice device, VkCommandPool command_pool) {
    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer command_buffer;
    VkResult result = vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkAllocateCommandBuffers() failed: %s\n", string_VkResult(result));
        return VK_NULL_HANDLE;
    }

    return command_buffer;
}

static VkBuffer create_host_buffer(VmaAllocator allocator, VkDeviceSize size,
                                   VmaAllocation *allocation, VmaAllocationInfo *allocation_info) {
    const VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    const VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
    };

    VkBuffer buffer;
    VkResult result =
        vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer, allocation, allocation_info);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vmaCreateBuffer() failed: %s\n", string_VkResult(result));
        return VK_NULL_HANDLE;
    }

    return buffer;
}

int main(void) {
    const VkDebugUtilsMessengerCreateInfoEXT debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,

        .pfnUserCallback = &vulkan_debug_callback,
    };

    VkInstance instance = create_instance(&debug_info);
    if (!instance) {
        fprintf(stderr, "create_instance() failed\n");
        return EXIT_FAILURE;
    }

    VkDebugUtilsMessengerEXT messenger = create_debug_messenger(instance, &debug_info);
    if (!messenger) {
        fprintf(stderr, "create_debug_messenger() failed\n");
        return EXIT_FAILURE;
    }

    Device device;
    if (!create_device(instance, &device)) {
        fprintf(stderr, "create_device() failed\n");
        return EXIT_FAILURE;
    }

    VkShaderModule shader = load_shader_module(device.device, "shaders/pathtracer.comp.spv");
    if (!shader) {
        fprintf(stderr, "load_shader_module() failed\n");
        return EXIT_FAILURE;
    }

    const Pathtracing_Pipeline_Info pipeline_info = {
        .compute_shader = shader,
        .workgroup_sizes =
            {
                .x = 8,
                .y = 4,
                .z = 1,
            },
    };

    Pathtracing_Pipeline pipeline;
    if (!create_pathtracing_pipeline(&device, &pipeline_info, &pipeline)) {
        fprintf(stderr, "create_pathtracing_pipeline() failed\n");
        return EXIT_FAILURE;
    }

    const uint32_t image_width = 512;
    const uint32_t image_height = 512;
    VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;

    VkDescriptorPool descriptor_pool = create_descriptor_pool(device.device);
    if (!descriptor_pool) {
        fprintf(stderr, "create_descriptor_pool() failed\n");
        return EXIT_FAILURE;
    }

    VkDescriptorSet descriptor_set =
        create_descriptor_set(device.device, descriptor_pool, pipeline.descriptor_set_layout);
    if (!descriptor_set) {
        fprintf(stderr, "create_descriptor_set() failed\n");
        return EXIT_FAILURE;
    }

    VmaAllocator allocator = create_vma_allocator(instance, device.device, &device.info);
    if (!allocator) {
        fprintf(stderr, "create_vma_allocator() failed\n");
        return EXIT_FAILURE;
    }

    VmaAllocation compute_image_allocation;
    VkImage compute_image = create_compute_image(allocator, image_width, image_height, image_format,
                                                 &compute_image_allocation);
    if (!compute_image) {
        fprintf(stderr, "create_compute_image() failed\n");
        return EXIT_FAILURE;
    }

    VkImageView compute_image_view =
        create_compute_image_view(device.device, compute_image, image_format);
    if (!compute_image_view) {
        fprintf(stderr, "create_compute_image_view() failed\n");
        return EXIT_FAILURE;
    }

    VmaAllocation host_buf_allocation;
    VmaAllocationInfo host_buf_alloc_info;
    VkBuffer host_buf =
        create_host_buffer(allocator, 4 * sizeof(uint32_t) * image_width * image_height,
                           &host_buf_allocation, &host_buf_alloc_info);
    if (!host_buf) {
        fprintf(stderr, "create_host_buffer() failed\n");
        return EXIT_FAILURE;
    }

    VkCommandPool command_pool = create_command_pool(device.device, &device.info);
    if (!command_pool) {
        fprintf(stderr, "create_command_pool() failed\n");
        return EXIT_FAILURE;
    }

    VkCommandBuffer command_buffer = create_command_buffer(device.device, command_pool);
    if (!command_buffer) {
        fprintf(stderr, "create_command_buffer() failed\n");
        return EXIT_FAILURE;
    }

    const VkWriteDescriptorSet write_descriptor_set = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo =
            &(VkDescriptorImageInfo){
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                .imageView = compute_image_view,
            },
    };

    vkUpdateDescriptorSets(device.device, 1, &write_descriptor_set, 0, NULL);

    VkResult record_result = vkBeginCommandBuffer(
        command_buffer, &(VkCommandBufferBeginInfo){
                            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                        });
    if (record_result != VK_SUCCESS) {
        fprintf(stderr, "vkBeginCommandBuffer() failed: %s\n", string_VkResult(record_result));
        return EXIT_FAILURE;
    }

    /* Transition from undefined to general for compute shader write operations. */
    const VkImageMemoryBarrier trans_to_general = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = compute_image,
        .subresourceRange =
            (VkImageSubresourceRange){
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1,
                         &trans_to_general);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0, 1,
                            &descriptor_set, 0, NULL);

    vkCmdDispatch(
        command_buffer,
        (image_width + pipeline_info.workgroup_sizes.x - 1) / pipeline_info.workgroup_sizes.x,
        (image_height + pipeline_info.workgroup_sizes.y - 1) / pipeline_info.workgroup_sizes.y,
        pipeline_info.workgroup_sizes.z);

    /* Transition from general to transfer src optimal for device -> host copy */
    const VkImageMemoryBarrier trans_to_trans_src = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = compute_image,
        .subresourceRange =
            (VkImageSubresourceRange){
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                         &trans_to_trans_src);

    const VkBufferImageCopy copy_region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .imageOffset = {0, 0, 0},
        .imageExtent = {image_width, image_height, 1},
    };

    vkCmdCopyImageToBuffer(command_buffer, compute_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           host_buf, 1, &copy_region);

    record_result = vkEndCommandBuffer(command_buffer);
    if (record_result != VK_SUCCESS) {
        fprintf(stderr, "vkEndCommandBuffer() failed: %s\n", string_VkResult(record_result));
        return EXIT_FAILURE;
    }

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pCommandBuffers = &command_buffer,
        .commandBufferCount = 1,
    };

    vkQueueSubmit(device.compute_queue, 1, &submit_info, NULL);
    vkQueueWaitIdle(device.compute_queue);

    uint8_t *data = host_buf_alloc_info.pMappedData;

    stbi_write_png("output.png", image_width, image_height, 4, data, 4 * image_width);

    vkDestroyCommandPool(device.device, command_pool, NULL);
    vkDestroyImageView(device.device, compute_image_view, NULL);
    vmaDestroyBuffer(allocator, host_buf, host_buf_allocation);
    vmaDestroyImage(allocator, compute_image, compute_image_allocation);
    vmaDestroyAllocator(allocator);
    vkDestroyDescriptorPool(device.device, descriptor_pool, NULL);
    destroy_pathtracing_pipeline(&device, &pipeline);
    vkDestroyShaderModule(device.device, shader, NULL);
    destroy_device(&device);
    DestroyDebugUtilsMessengerEXT(instance, messenger, NULL);
    vkDestroyInstance(instance, NULL);
    return EXIT_SUCCESS;
}
