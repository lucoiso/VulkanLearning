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
    constexpr char const* g_EntryPoint = "main";

    class VulkanShaderManager final // NOLINT(cppcoreguidelines-special-member-functions)
    {
    public:
        VulkanShaderManager();

        VulkanShaderManager(VulkanShaderManager const&)            = delete;
        VulkanShaderManager& operator=(VulkanShaderManager const&) = delete;

        ~VulkanShaderManager();

        void Shutdown();

        static VulkanShaderManager& Get();

        bool Compile(std::string_view Source, std::vector<uint32_t>& OutSPIRVCode);
        static bool Load(std::string_view Source, std::vector<uint32_t>& OutSPIRVCode);

        bool CompileOrLoadIfExists(std::string_view Source, std::vector<uint32_t>& OutSPIRVCode);

        VkShaderModule CreateModule(VkDevice const& Device, std::vector<uint32_t> const& SPIRVCode, EShLanguage Language);

        [[nodiscard]] VkPipelineShaderStageCreateInfo GetStageInfo(VkShaderModule const& Module) const;
        [[nodiscard]] std::vector<VkShaderModule> GetShaderModules() const;
        [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> GetStageInfos() const;

        void FreeStagedModules(std::vector<VkPipelineShaderStageCreateInfo> const& StagedModules);

    private:
        bool Compile(std::string_view Source, EShLanguage Language, std::vector<uint32_t>& OutSPIRVCode);

        void StageInfo(VkShaderModule const& Module, EShLanguage Language);

        std::unordered_map<VkShaderModule, VkPipelineShaderStageCreateInfo> m_StageInfos;
    };
}
