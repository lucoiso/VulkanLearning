// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"

namespace RenderCore
{
class VulkanPipeline
{
    public:
        VulkanPipeline();

        VulkanPipeline(const VulkanPipeline&) = delete;
        VulkanPipeline& operator=(const VulkanPipeline&) = delete;

        ~VulkanPipeline();
    };
}
