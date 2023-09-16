// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include <vulkan/vulkan_core.h>
#include <glslang/Public/ShaderLang.h>
#include <vector>
#include <string_view>
#include <unordered_map>

struct GLFWwindow;

namespace RenderCore
{
    constexpr const char *EntryPoint = "main";

    class VulkanShaderManager
    {
    public:
        VulkanShaderManager();
        ~VulkanShaderManager();

        void Shutdown();

        bool Compile(const std::string_view Source, std::vector<uint32_t> &OutSPIRVCode);
        bool Load(const std::string_view Source, std::vector<uint32_t> &OutSPIRVCode);
        
        bool CompileOrLoadIfExists(const std::string_view Source, std::vector<uint32_t> &OutSPIRVCode);

        VkShaderModule CreateModule(const VkDevice &Device, const std::vector<uint32_t> &SPIRVCode, EShLanguage Language);
        VkPipelineShaderStageCreateInfo GetStageInfo(const VkShaderModule &Module);

        std::vector<VkShaderModule> GetShaderModules() const;
        std::vector<VkPipelineShaderStageCreateInfo> GetStageInfos() const;

        void FreeStagedModules(const std::vector<VkPipelineShaderStageCreateInfo> &StagedModules);

    private:
        bool Compile(const std::string_view Source, EShLanguage Language, std::vector<uint32_t> &OutSPIRVCode);
        void StageInfo(const VkShaderModule &Module, EShLanguage Language);

        std::unordered_map<VkShaderModule, VkPipelineShaderStageCreateInfo> m_StageInfos;
    };
}
