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

    DestroyDebugUtilsMessengerEXT(instance, messenger, NULL);
    vkDestroyInstance(instance, NULL);
    return EXIT_SUCCESS;
}
