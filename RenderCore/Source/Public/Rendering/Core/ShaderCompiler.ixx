// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <glslang/Public/ShaderLang.h>

export module RenderCore.Runtime.ShaderCompiler;

namespace RenderCore
{
    export struct ShaderStageData
    {
        VkPipelineShaderStageCreateInfo StageInfo {};
        std::vector<uint32_t>           ShaderCode {};
    };

    RENDERCOREMODULE_API std::vector<ShaderStageData> g_StageInfos;
}

export namespace RenderCore
{
    enum class ShaderType
    {
        GLSL,
        HLSL
    };

    RENDERCOREMODULE_API [[nodiscard]] bool Compile(strzilla::string_view, ShaderType, strzilla::string_view, std::int32_t, EShLanguage, std::vector<uint32_t> &);
    RENDERCOREMODULE_API [[nodiscard]] bool Load(strzilla::string_view, std::vector<uint32_t> &);
    RENDERCOREMODULE_API [[nodiscard]] bool CompileOrLoadIfExists(strzilla::string_view,
                                             ShaderType,
                                             strzilla::string_view,
                                             std::int32_t,
                                             EShLanguage,
                                             std::vector<uint32_t> &);


    RENDERCOREMODULE_API [[nodiscard]] inline std::vector<ShaderStageData> const &GetStageData()
    {
        return g_StageInfos;
    }

    inline void ReleaseShaderResources()
    {
        g_StageInfos.clear();
    }

    void CompileDefaultShaders();
} // namespace RenderCore
