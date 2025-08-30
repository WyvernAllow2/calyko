#include "device.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vk_enum_string_helper.h>

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

static void get_physical_device_info(VkPhysicalDevice physical_device,
                                     Physical_Device_Info *out_info) {
    assert(out_info);
    assert(physical_device);

    *out_info = (Physical_Device_Info){
        .physical_device = physical_device,
    };

    vkGetPhysicalDeviceMemoryProperties(physical_device, &out_info->memory_properties);
    vkGetPhysicalDeviceProperties(physical_device, &out_info->properties);

    out_info->compute_family_index = find_compute_queue_index(out_info->physical_device);
}

static int rate_physical_device(const Physical_Device_Info *info) {
    int score = 0;
    if (info->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 100;
    }

    return score;
}

static bool find_best_physical_device(VkInstance instance, Physical_Device_Info *out_info) {
    assert(instance);
    assert(out_info);

    *out_info = (Physical_Device_Info){0};

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    VkPhysicalDevice *physical_devices = malloc(sizeof(*physical_devices) * device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);

    int best_score = INT_MIN;
    for (uint32_t i = 0; i < device_count; i++) {
        Physical_Device_Info info;
        get_physical_device_info(physical_devices[i], &info);

        int score = rate_physical_device(&info);
        if (score > best_score) {
            best_score = score;
            *out_info = info;
        }
    }

    free(physical_devices);
    return out_info->physical_device != NULL;
}

bool create_device(VkInstance instance, Device *dev) {
    assert(instance);
    assert(dev);

    if (!find_best_physical_device(instance, &dev->info)) {
        fprintf(stderr, "Failed to find a suitable physical device\n");
        return false;
    }

    const float queue_priority = 1.0f;
    const VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = dev->info.compute_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queue_info,
        .queueCreateInfoCount = 1,
    };

    VkResult result = vkCreateDevice(dev->info.physical_device, &device_info, NULL, &dev->device);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDevice() failed: %s\n", string_VkResult(result));
        return false;
    }

    vkGetDeviceQueue(dev->device, dev->info.compute_family_index, 0, &dev->compute_queue);
    return true;
}

void destroy_device(Device *device) {
    vkDestroyDevice(device->device, NULL);
}
