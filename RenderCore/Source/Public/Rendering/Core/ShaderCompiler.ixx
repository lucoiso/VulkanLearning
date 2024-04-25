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

    struct ShaderStageData
    {
        VkPipelineShaderStageCreateInfo StageInfo {};
        std::vector<uint32_t>           ShaderCode {};
    };

    [[nodiscard]] bool Compile(std::string_view, ShaderType, std::string_view, std::int32_t, EShLanguage, std::vector<uint32_t> &);
    [[nodiscard]] bool Load(std::string_view, std::vector<uint32_t> &);
    [[nodiscard]] bool CompileOrLoadIfExists(std::string_view, ShaderType, std::string_view, std::int32_t, EShLanguage, std::vector<uint32_t> &);

    [[nodiscard]] std::vector<ShaderStageData> const &GetStageData();
    void                                              ReleaseShaderResources();

    void CompileDefaultShaders();
} // namespace RenderCore
