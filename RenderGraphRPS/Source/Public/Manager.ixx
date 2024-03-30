// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include "RenderGraphRPSModule.hpp"

export module RenderGraphRPS.Manager;

namespace RenderGraphRPS
{
    export RENDERGRAPHRPSMODULE_API bool CreateRPSDevice(VkDevice const &, VkPhysicalDevice const &);
} // namespace RenderGraphRPS
