// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include <vector>
#include <unordered_map>
#include <glslang/Public/ShaderLang.h>
#include <volk.h>

namespace RenderCore
{
    constexpr const char* g_EntryPoint = "main";

    class VulkanShaderManager final // NOLINT(cppcoreguidelines-special-member-functions)
    {
    public:
        VulkanShaderManager(const VulkanShaderManager&)            = delete;
        VulkanShaderManager& operator=(const VulkanShaderManager&) = delete;

        VulkanShaderManager();
        ~VulkanShaderManager();

        void Shutdown();

        static VulkanShaderManager& Get();

        bool Compile(std::string_view Source, std::vector<uint32_t>& OutSPIRVCode);
        bool Load(std::string_view Source, std::vector<uint32_t>& OutSPIRVCode) const;

        bool CompileOrLoadIfExists(std::string_view Source, std::vector<uint32_t>& OutSPIRVCode);

        // ReSharper disable once CppFunctionIsNotImplemented
        VkShaderModule CreateModule(const VkDevice& Device, const std::vector<uint32_t>& SPIRVCode, EShLanguage Language);

        [[nodiscard]] VkPipelineShaderStageCreateInfo GetStageInfo(const VkShaderModule& Module) const;
        [[nodiscard]] std::vector<VkShaderModule> GetShaderModules() const;
        [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> GetStageInfos() const;

        void FreeStagedModules(const std::vector<VkPipelineShaderStageCreateInfo>& StagedModules);

    private:
        static VulkanShaderManager g_Instance;

        // ReSharper disable once CppFunctionIsNotImplemented
        static bool Compile(std::string_view Source, EShLanguage Language, std::vector<uint32_t>& OutSPIRVCode);

        // ReSharper disable once CppFunctionIsNotImplemented
        void StageInfo(const VkShaderModule& Module, EShLanguage Language);

        std::unordered_map<VkShaderModule, VkPipelineShaderStageCreateInfo> m_StageInfos;
    };
}
