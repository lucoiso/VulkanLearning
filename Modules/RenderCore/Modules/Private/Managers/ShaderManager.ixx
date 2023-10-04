// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <glslang/Public/ShaderLang.h>
#include <volk.h>

export module RenderCore.Managers.ShaderManager;

import <unordered_map>;
import <vector>;
import <ranges>;
import <format>;
import <fstream>;
import <sstream>;
import <string>;
import <string_view>;

export namespace RenderCore
{
    constexpr char const* g_EntryPoint = "main";

    class ShaderManager final
    {
    public:
        ShaderManager();

        ShaderManager(ShaderManager const&)            = delete;
        ShaderManager& operator=(ShaderManager const&) = delete;

        ~ShaderManager();

        void Shutdown();

        static ShaderManager& Get();

        static bool Compile(std::string_view Source, std::vector<uint32_t>& OutSPIRVCode);
        static bool Load(std::string_view Source, std::vector<uint32_t>& OutSPIRVCode);

        static bool CompileOrLoadIfExists(std::string_view Source, std::vector<uint32_t>& OutSPIRVCode);

        VkShaderModule CreateModule(VkDevice const& Device, std::vector<uint32_t> const& SPIRVCode, EShLanguage Language);

        [[nodiscard]] VkPipelineShaderStageCreateInfo GetStageInfo(VkShaderModule const& Module) const;
        [[nodiscard]] std::vector<VkShaderModule> GetShaderModules() const;
        [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> GetStageInfos() const;

        void FreeStagedModules(std::vector<VkPipelineShaderStageCreateInfo> const& StagedModules);

    private:
        static bool Compile(std::string_view Source, EShLanguage Language, std::vector<uint32_t>& OutSPIRVCode);

        void StageInfo(VkShaderModule const& Module, EShLanguage Language);

        std::unordered_map<VkShaderModule, VkPipelineShaderStageCreateInfo> m_StageInfos;
    };
}// namespace RenderCore