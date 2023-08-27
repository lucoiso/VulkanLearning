// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"
#include <vulkan/vulkan.h>
#include <glslang/Public/ShaderLang.h>
#include <memory>
#include <vector>
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

        bool CompileShader(const char* ShaderSource, std::vector<uint32_t>& OutSPIRVCode);
        bool LoadShader(const char* ShaderSource, std::vector<uint32_t>& OutSPIRVCode);

        VkShaderModule CreateShaderModule(const VkDevice& Device, const std::vector<uint32_t>& SPIRVCode, EShLanguage ShaderLanguage);

    private:
        bool CompileShader(const char* ShaderSource, EShLanguage ShaderLanguage, std::vector<uint32_t>& OutSPIRVCode);
        void StageShaderInfo(const VkShaderModule& Module, EShLanguage ShaderLanguage);

        std::unordered_map<VkShaderModule, VkPipelineShaderStageCreateInfo> m_ShaderStageInfos;
    };
}
