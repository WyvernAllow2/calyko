#ifndef DEVICE_H
#define DEVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

typedef struct Physical_Device_Info {
    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;

    uint32_t compute_family_index;
} Physical_Device_Info;

typedef struct Device {
    Physical_Device_Info info;

    VkDevice device;
    VkQueue compute_queue;
} Device;

bool create_device(VkInstance instance, Device *dev);
void destroy_device(Device *dev);

#endif /* DEVICE_H */
