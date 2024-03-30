// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <rps/rps.h>
#include "RenderGraphRPSModule.hpp"

export module RenderGraphRPS.Manager;

namespace RenderGraphRPS
{
    export RENDERGRAPHRPSMODULE_API bool CreateRPSDevice(VkDevice const &, VkPhysicalDevice const &);
    export RENDERGRAPHRPSMODULE_API void DestroyDevice();

    export RENDERGRAPHRPSMODULE_API RpsRenderGraph CreateRPSRenderGraph(RpsRpslEntry const &);
    export RENDERGRAPHRPSMODULE_API void           DestroyRenderGraph(RpsRenderGraph const &);

    export RENDERGRAPHRPSMODULE_API void RecordRenderGraphCommands(RpsRenderGraph const &, VkCommandBuffer const &);
} // namespace RenderGraphRPS
