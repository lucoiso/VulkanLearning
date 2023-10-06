// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <glslang/Public/ShaderLang.h>
#include <volk.h>

export module RenderCore.Management.ShaderManagement;

import <vector>;
import <string_view>;

export namespace RenderCore
{
    [[nodiscard]] bool Compile(std::string_view, std::vector<uint32_t>&);
    [[nodiscard]] bool Load(std::string_view, std::vector<uint32_t>&);
    [[nodiscard]] bool CompileOrLoadIfExists(std::string_view, std::vector<uint32_t>&);
    [[nodiscard]] VkShaderModule CreateModule(VkDevice const&, std::vector<uint32_t> const&, EShLanguage);

    [[nodiscard]] VkPipelineShaderStageCreateInfo GetStageInfo(VkShaderModule const& Module);
    [[nodiscard]] std::vector<VkShaderModule> GetShaderModules();
    [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> GetStageInfos();

    void ReleaseShaderResources();
    void FreeStagedModules(std::vector<VkPipelineShaderStageCreateInfo> const&);
}// namespace RenderCore