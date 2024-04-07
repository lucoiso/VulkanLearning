// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <cstdint>
#include <glslang/Public/ShaderLang.h>
#include <string>
#include <vector>

export module RenderCore.Runtime.ShaderCompiler;

export namespace RenderCore
{
    enum class ShaderType
    {
        GLSL,
        HLSL
    };

    [[nodiscard]] bool Compile(std::string_view, ShaderType, std::string_view, std::int32_t, EShLanguage, std::vector<uint32_t> &);
    [[nodiscard]] bool Load(std::string_view, std::vector<uint32_t> &);
    [[nodiscard]] bool CompileOrLoadIfExists(std::string_view, ShaderType, std::string_view, std::int32_t, EShLanguage, std::vector<uint32_t> &);
    [[nodiscard]] VkShaderModule CreateModule(std::vector<uint32_t> const &, EShLanguage, std::string_view);

    [[nodiscard]] VkPipelineShaderStageCreateInfo              GetStageInfo(VkShaderModule const &Module);
    [[nodiscard]] std::vector<VkShaderModule>                  GetShaderModules();
    [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> GetStageInfos();

    void ReleaseShaderResources();
    void FreeStagedModules(std::vector<VkPipelineShaderStageCreateInfo> const &);

    [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> CompileDefaultShaders();
} // namespace RenderCore
