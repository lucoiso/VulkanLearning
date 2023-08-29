// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"
#include <vulkan/vulkan.h>
#include <glslang/Public/ShaderLang.h>
#include <vector>
#include <string_view>
#include <unordered_map>

struct GLFWwindow;

namespace RenderCore
{
    constexpr const char* EntryPoint = "main";

    class VulkanShaderCompiler
    {
    public:
        VulkanShaderCompiler();
        ~VulkanShaderCompiler();

        bool Compile(const std::string_view Source, std::vector<uint32_t>& OutSPIRVCode);
        bool Load(const std::string_view Source, std::vector<uint32_t>& OutSPIRVCode);

        VkShaderModule CreateModule(const VkDevice& Device, const std::vector<uint32_t>& SPIRVCode, EShLanguage Language);

    private:
        bool Compile(const std::string_view Source, EShLanguage Language, std::vector<uint32_t>& OutSPIRVCode);
        void StageInfo(const VkShaderModule& Module, EShLanguage Language);

        std::unordered_map<VkShaderModule, VkPipelineShaderStageCreateInfo> m_StageInfos;
    };
}
