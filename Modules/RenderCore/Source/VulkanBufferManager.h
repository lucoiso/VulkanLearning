// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"

namespace RenderCore
{
    class RENDERCOREMODULE_API VulkanBufferManager
    {
    public:
        VulkanBufferManager();

        VulkanBufferManager(const VulkanBufferManager&) = delete;
        VulkanBufferManager& operator=(const VulkanBufferManager&) = delete;

        ~VulkanBufferManager();
    };
}
