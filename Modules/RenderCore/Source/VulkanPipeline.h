// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"

namespace RenderCore
{
class RENDERCOREMODULE_API VulkanPipeline
{
    public:
        VulkanPipeline();

        VulkanPipeline(const VulkanPipeline&) = delete;
        VulkanPipeline& operator=(const VulkanPipeline&) = delete;

        ~VulkanPipeline();
    };
}
