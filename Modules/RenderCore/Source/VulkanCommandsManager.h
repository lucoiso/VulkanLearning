// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"
#include <vulkan/vulkan.h>

namespace RenderCore
{
class VulkanCommandsManager
{
    public:
        VulkanCommandsManager();

        VulkanCommandsManager(const VulkanCommandsManager&) = delete;
        VulkanCommandsManager& operator=(const VulkanCommandsManager&) = delete;

        ~VulkanCommandsManager();
    };
}
