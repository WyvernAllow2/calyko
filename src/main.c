#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))

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

static uint32_t find_compute_queue_index(VkPhysicalDevice physical_device) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
    VkQueueFamilyProperties *properties = malloc(sizeof(*properties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, properties);

    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            free(properties);
            return i;
        }
    }

    free(properties);

    /* Should be unreachable on conformant implementations; the Vulkan specification
     * guarantees that every device has at least one queue family with VK_QUEUE_COMPUTE_BIT.
     */
    assert(!"Could not find a queue family capable of compute");
    return 0;
}

typedef struct Device_Info {
    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;

    uint32_t compute_family_index;
} Device_Info;

static void get_device_info(VkPhysicalDevice physical_device, Device_Info *info) {
    assert(info);
    *info = (Device_Info){
        .physical_device = physical_device,
    };

    vkGetPhysicalDeviceProperties(physical_device, &info->properties);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &info->memory_properties);
    info->compute_family_index = find_compute_queue_index(info->physical_device);
}

static int rate_physical_device(const Device_Info *info) {
    int score = 0;
    if (info->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 100;
    }

    return score;
}

static Device_Info find_best_device(VkInstance instance) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    VkPhysicalDevice *physical_devices = malloc(sizeof(*physical_devices) * device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);

    int best_score = INT_MIN;
    Device_Info best_device = {0};

    for (uint32_t i = 0; i < device_count; i++) {
        Device_Info info;
        get_device_info(physical_devices[i], &info);

        int score = rate_physical_device(&info);
        if (score > best_score) {
            best_score = score;
            best_device = info;
        }
    }

    free(physical_devices);
    return best_device;
}

static VkDevice create_device(const Device_Info *info) {
    const float queue_priority = 1.0f;
    const VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = info->compute_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queue_info,
        .queueCreateInfoCount = 1,
    };

    VkDevice device = NULL;
    VkResult result = vkCreateDevice(info->physical_device, &device_info, NULL, &device);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDevice() failed: %s\n", string_VkResult(result));
        return NULL;
    }

    return device;
}

static uint32_t *read_file_bytes(const char *filename, uint32_t *len) {
    *len = 0;

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror(filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(file);
    rewind(file);

    uint32_t *bytes = malloc(file_size);
    if (!bytes) {
        perror("malloc() failed");
        fclose(file);
        return NULL;
    }

    if (fread(bytes, sizeof(*bytes), file_size, file) != file_size / sizeof(*bytes)) {
        perror("fread() failed");
        free(bytes);
        fclose(file);
        return NULL;
    }

    *len = file_size;
    return bytes;
}

static VkShaderModule load_shader_module(VkDevice device, const char *filename) {
    uint32_t code_len;
    uint32_t *code = read_file_bytes(filename, &code_len);
    if (!code) {
        fprintf(stderr, "read_file_bytes failed\n");
        return VK_NULL_HANDLE;
    }

    const VkShaderModuleCreateInfo shader_module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pCode = code,
        .codeSize = code_len,
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

static VkDescriptorSetLayout create_descriptor_set_layout(VkDevice device) {
    const VkDescriptorSetLayoutBinding bindings[] = {
        (VkDescriptorSetLayoutBinding){
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };

    const VkDescriptorSetLayoutCreateInfo set_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pBindings = bindings,
        .bindingCount = ARRAY_LEN(bindings),
    };

    VkDescriptorSetLayout layout;
    VkResult result = vkCreateDescriptorSetLayout(device, &set_layout_info, NULL, &layout);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDescriptorSetLayout() failed: %s\n", string_VkResult(result));
        return VK_NULL_HANDLE;
    }

    return layout;
}

static VkPipelineLayout create_pipeline_layout(VkDevice device, VkDescriptorSetLayout set_layout) {
    const VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pSetLayouts = &set_layout,
        .setLayoutCount = 1,
    };

    VkPipelineLayout layout;
    VkResult result = vkCreatePipelineLayout(device, &pipeline_layout_info, NULL, &layout);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreatePipelineLayout failed: %s\n", string_VkResult(result));
        return VK_NULL_HANDLE;
    }

    return layout;
}

typedef struct WorkgroupSizes {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} WorkgroupSizes;

static VkPipeline create_compute_pipeline(VkDevice device, VkPipelineLayout layout,
                                          VkShaderModule module, WorkgroupSizes workgroup_sizes) {
    const VkSpecializationMapEntry entries[] = {
        (VkSpecializationMapEntry){
            .constantID = 0,
            .offset = offsetof(WorkgroupSizes, x),
            sizeof(uint32_t),
        },

        (VkSpecializationMapEntry){
            .constantID = 1,
            .offset = offsetof(WorkgroupSizes, y),
            sizeof(uint32_t),
        },

        (VkSpecializationMapEntry){
            .constantID = 2,
            .offset = offsetof(WorkgroupSizes, y),
            sizeof(uint32_t),
        },
    };

    const VkSpecializationInfo specialization_info = {
        .pMapEntries = entries,
        .mapEntryCount = ARRAY_LEN(entries),
        .dataSize = sizeof(WorkgroupSizes),
        .pData = &workgroup_sizes,
    };

    const VkPipelineShaderStageCreateInfo shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module = module,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .pName = "main",
        .pSpecializationInfo = &specialization_info,
    };

    const VkComputePipelineCreateInfo compute_pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .layout = layout,
        .stage = shader_stage_info,
    };

    VkPipeline pipeline;
    VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute_pipeline_info,
                                               NULL, &pipeline);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateComputePipelines() failed: %s\n", string_VkResult(result));
        return VK_NULL_HANDLE;
    }

    return pipeline;
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

    Device_Info device_info = find_best_device(instance);
    if (!device_info.physical_device) {
        fprintf(stderr, "Failed to find a suitable physical device\n");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Found a suitable physical device: %s\n", device_info.properties.deviceName);

    VkDevice device = create_device(&device_info);
    if (!device) {
        fprintf(stderr, "create_device() failed\n");
        return EXIT_FAILURE;
    }

    VkQueue compute_queue;
    vkGetDeviceQueue(device, device_info.compute_family_index, 0, &compute_queue);

    VkDescriptorSetLayout descriptor_set_layout = create_descriptor_set_layout(device);
    if (!descriptor_set_layout) {
        fprintf(stderr, "create_descriptor_set_layout() failed\n");
        return EXIT_FAILURE;
    }

    VkPipelineLayout pipeline_layout = create_pipeline_layout(device, descriptor_set_layout);
    if (!descriptor_set_layout) {
        fprintf(stderr, "create_pipeline_layout() failed\n");
        return EXIT_FAILURE;
    }

    VkShaderModule shader = load_shader_module(device, "shaders/pathtracer.comp.spv");
    if (!shader) {
        fprintf(stderr, "load_shader_module() failed\n");
        return EXIT_FAILURE;
    }

    VkPipeline pipeline =
        create_compute_pipeline(device, pipeline_layout, shader, (WorkgroupSizes){8, 8, 1});
    if (!pipeline) {
        fprintf(stderr, "create_pipeline_layout() failed\n");
        return EXIT_FAILURE;
    }

    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyShaderModule(device, shader, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);
    vkDestroyDevice(device, NULL);
    DestroyDebugUtilsMessengerEXT(instance, messenger, NULL);
    vkDestroyInstance(instance, NULL);
    return EXIT_SUCCESS;
}
