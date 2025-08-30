#ifndef PIPELINE_H
#define PIPELINE_H

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "device.h"

typedef struct Workgroup_Sizes {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} Workgroup_Sizes;

typedef struct Pathtracing_Pipeline_Info {
    VkShaderModule compute_shader;
    Workgroup_Sizes workgroup_sizes;
} Pathtracing_Pipeline_Info;

typedef struct Pathtracing_Pipeline {
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout layout;
    VkPipeline pipeline;
} Pathtracing_Pipeline;

bool create_pathtracing_pipeline(const Device *device, const Pathtracing_Pipeline_Info *info,
                                 Pathtracing_Pipeline *pipeline);
void destroy_pathtracing_pipeline(const Device *device, Pathtracing_Pipeline *pipeline);

#endif /* PIPELINE_H */
