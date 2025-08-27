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
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
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

    DestroyDebugUtilsMessengerEXT(instance, messenger, NULL);
    vkDestroyInstance(instance, NULL);
    return EXIT_SUCCESS;
}
