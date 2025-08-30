#include "pipeline.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#include "device.h"
#include "utils.h"

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

static VkPipeline create_pipeline(VkDevice device, VkPipelineLayout layout,
                                  const Pathtracing_Pipeline_Info *info) {
    const VkSpecializationMapEntry entries[] = {
        (VkSpecializationMapEntry){
            .constantID = 0,
            .offset = offsetof(Workgroup_Sizes, x),
            sizeof(uint32_t),
        },

        (VkSpecializationMapEntry){
            .constantID = 1,
            .offset = offsetof(Workgroup_Sizes, y),
            sizeof(uint32_t),
        },

        (VkSpecializationMapEntry){
            .constantID = 2,
            .offset = offsetof(Workgroup_Sizes, y),
            sizeof(uint32_t),
        },
    };

    const VkSpecializationInfo specialization_info = {
        .pMapEntries = entries,
        .mapEntryCount = ARRAY_LEN(entries),
        .dataSize = sizeof(Workgroup_Sizes),
        .pData = &info->workgroup_sizes,
    };

    const VkPipelineShaderStageCreateInfo shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module = info->compute_shader,
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

bool create_pathtracing_pipeline(const Device *device, const Pathtracing_Pipeline_Info *info,
                                 Pathtracing_Pipeline *pipeline) {
    assert(device);
    assert(info);
    assert(pipeline);

    pipeline->descriptor_set_layout = create_descriptor_set_layout(device->device);
    if (!pipeline->descriptor_set_layout) {
        fprintf(stderr, "create_descriptor_set_layout() failed\n");
        return false;
    }

    pipeline->layout = create_pipeline_layout(device->device, pipeline->descriptor_set_layout);
    if (!pipeline->layout) {
        fprintf(stderr, "create_pipeline_layout() failed\n");
        return false;
    }

    pipeline->pipeline = create_pipeline(device->device, pipeline->layout, info);
    if (!pipeline->pipeline) {
        fprintf(stderr, "create_pipeline() failed\n");
        return false;
    }

    return true;
}

void destroy_pathtracing_pipeline(const Device *device, Pathtracing_Pipeline *pipeline) {
    vkDestroyPipeline(device->device, pipeline->pipeline, NULL);
    vkDestroyPipelineLayout(device->device, pipeline->layout, NULL);
    vkDestroyDescriptorSetLayout(device->device, pipeline->descriptor_set_layout, NULL);
}
