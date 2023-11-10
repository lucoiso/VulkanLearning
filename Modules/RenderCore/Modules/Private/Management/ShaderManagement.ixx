// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <glslang/Public/ShaderLang.h>
#include <volk.h>

export module RenderCore.Management.ShaderManagement;

export import <string_view>;
export import <vector>;
export import <cstdint>;

namespace RenderCore
{
    export [[nodiscard]] bool Compile(std::string_view const&, std::vector<uint32_t>&);
    export [[nodiscard]] bool Load(std::string_view const&, std::vector<uint32_t>&);
    export [[nodiscard]] bool CompileOrLoadIfExists(std::string_view const&, std::vector<uint32_t>&);
    export [[nodiscard]] VkShaderModule CreateModule(VkDevice const&, std::vector<uint32_t> const&, EShLanguage);

    export [[nodiscard]] VkPipelineShaderStageCreateInfo GetStageInfo(VkShaderModule const& Module);
    export [[nodiscard]] std::vector<VkShaderModule> GetShaderModules();
    export [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> GetStageInfos();

    export void ReleaseShaderResources();
    export void FreeStagedModules(std::vector<VkPipelineShaderStageCreateInfo> const&);
}// namespace RenderCore