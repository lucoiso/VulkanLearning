// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"

namespace RenderCore
{
    class VulkanBufferManager
    {
    public:
        VulkanBufferManager();

        VulkanBufferManager(const VulkanBufferManager&) = delete;
        VulkanBufferManager& operator=(const VulkanBufferManager&) = delete;

        ~VulkanBufferManager();
    };
}
