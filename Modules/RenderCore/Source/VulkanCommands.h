// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"

namespace RenderCore
{
class RENDERCOREMODULE_API VulkanCommands
{
    public:
        VulkanCommands();

        VulkanCommands(const VulkanCommands&) = delete;
        VulkanCommands& operator=(const VulkanCommands&) = delete;

        ~VulkanCommands();
    };
}
