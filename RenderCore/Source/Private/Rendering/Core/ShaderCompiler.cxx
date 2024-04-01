// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <array>
#include <boost/log/trivial.hpp>
#include <filesystem>
#include <fstream>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/SPIRV/Logger.h>
#include <ranges>

#ifdef _DEBUG
    #include <spirv-tools/libspirv.hpp>
#endif

module RenderCore.Runtime.ShaderCompiler;

import RenderCore.Utils.Helpers;
import RenderCore.Utils.DebugHelpers;
import RuntimeInfo.Manager;

using namespace RenderCore;

std::unordered_map<VkShaderModule, VkPipelineShaderStageCreateInfo> g_StageInfos {};

bool CompileInternal(ShaderType const            ShaderType,
                     std::string_view const      Source,
                     EShLanguage const           Language,
                     std::string_view const      EntryPoint,
                     std::int32_t const          Version,
                     std::vector<std::uint32_t> &OutSPIRVCode)
{
    glslang::InitializeProcess();

    glslang::TShader Shader(Language);

    char const *ShaderContent = std::data(Source);
    Shader.setStringsWithLengths(&ShaderContent, nullptr, 1);

    Shader.setEntryPoint(std::data(EntryPoint));
    Shader.setSourceEntryPoint(std::data(EntryPoint));
    Shader.setEnvInput(ShaderType == ShaderType::GLSL ? glslang::EShSourceGlsl : glslang::EShSourceHlsl, Language, glslang::EShClientVulkan, 1);
    Shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    Shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    TBuiltInResource const *Resources    = GetDefaultResources();
    constexpr auto          MessageFlags = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if (!Shader.parse(Resources, Version, ECoreProfile, false, true, MessageFlags))
    {
        glslang::FinalizeProcess();
        std::string const InfoLog("Info Log: " + std::string(Shader.getInfoLog()));
        std::string const DebugLog("Debug Log: " + std::string(Shader.getInfoDebugLog()));
        BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: " << std::format("Failed to parse shader:\n{}\n{}", InfoLog, DebugLog);
        return false;
    }

    glslang::TProgram Program;
    Program.addShader(&Shader);

    if (!Program.link(MessageFlags))
    {
        glslang::FinalizeProcess();
        std::string const InfoLog("Info Log: " + std::string(Program.getInfoLog()));
        std::string const DebugLog("Debug Log: " + std::string(Program.getInfoDebugLog()));
        BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: " << std::format("Failed to parse shader:\n{}\n{}", InfoLog, DebugLog);
        return false;
    }

#ifdef _DEBUG
    if constexpr (g_EnableCustomDebug)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Compiling shader:\n" << Source;
    }
#endif

    spv::SpvBuildLogger Logger;
    GlslangToSpv(*Program.getIntermediate(Language), OutSPIRVCode, &Logger);
    glslang::FinalizeProcess();

    if (std::string const GeneratedLogs = Logger.getAllMessages(); !std::empty(GeneratedLogs))
    {
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Shader compilation result log:\n" << GeneratedLogs;
    }

    return !std::empty(OutSPIRVCode);
}

void StageInfo(VkShaderModule const &Module, EShLanguage const Language, std::string_view const EntryPoint)
{
    VkPipelineShaderStageCreateInfo StageInfo {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .module = Module, .pName = std::data(EntryPoint)};

    switch (Language)
    {
        case EShLangVertex:
            StageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            break;

        case EShLangFragment:
            StageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;

        case EShLangCompute:
            StageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            break;

        case EShLangGeometry:
            StageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
            break;

        case EShLangTessControl:
            StageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            break;

        case EShLangTessEvaluation:
            StageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            break;

        case EShLangRayGen:
            StageInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            break;

        case EShLangIntersect:
            StageInfo.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            break;

        case EShLangAnyHit:
            StageInfo.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            break;

        case EShLangClosestHit:
            StageInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            break;

        case EShLangMiss:
            StageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
            break;

        case EShLangCallable:
            StageInfo.stage = VK_SHADER_STAGE_CALLABLE_BIT_KHR;
            break;

        default:
            break;
    }

    g_StageInfos.emplace(Module, StageInfo);
}

#ifdef _DEBUG
bool ValidateSPIRV(const std::vector<std::uint32_t> &SPIRVData)
{
    static spvtools::SpirvTools SPIRVToolsInstance(SPV_ENV_VULKAN_1_3);
    if (!SPIRVToolsInstance.IsValid())
    {
        return false;
    }

    if constexpr (g_EnableCustomDebug)
    {
        if (std::string DisassemblySPIRVData; SPIRVToolsInstance.Disassemble(SPIRVData, &DisassemblySPIRVData))
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Generated SPIR-V shader disassembly:\n" << DisassemblySPIRVData;
        }
    }

    return SPIRVToolsInstance.Validate(SPIRVData);
}
#endif

bool RenderCore::Compile(std::string_view const      Source,
                         ShaderType const            ShaderType,
                         std::string_view const      EntryPoint,
                         std::int32_t const          Version,
                         EShLanguage const           Language,
                         std::vector<std::uint32_t> &OutSPIRVCode)
{
    std::filesystem::path const Path(Source);
    std::stringstream           ShaderSource;
    std::ifstream               File(Path);

    if (!File.is_open())
    {
        return false;
    }

    ShaderSource << File.rdbuf();
    File.close();

    bool const Result = CompileInternal(ShaderType, ShaderSource.str(), Language, EntryPoint, Version, OutSPIRVCode);

    if (Result)
    {
#ifdef _DEBUG
        if (!ValidateSPIRV(OutSPIRVCode))
        {
            return false;
        }
#endif

        std::string const SPIRVPath = std::format("{}_{}.spv", Source, static_cast<std::uint8_t>(Language));
        std::ofstream     SPIRVFile(SPIRVPath, std::ios::binary);
        if (!SPIRVFile.is_open())
        {
            return false;
        }

        SPIRVFile << std::string(reinterpret_cast<char const *>(std::data(OutSPIRVCode)), std::size(OutSPIRVCode) * sizeof(std::uint32_t));
        SPIRVFile.close();
    }

    return Result;
}

bool RenderCore::Load(std::string_view const Source, std::vector<std::uint32_t> &OutSPIRVCode)
{
    std::filesystem::path const Path(Source);
    if (!exists(Path))
    {
        return false;
    }

    std::ifstream File(Path, std::ios::ate | std::ios::binary);
    if (!File.is_open())
    {
        return false;
    }

    std::size_t const FileSize = File.tellg();
    if (FileSize == 0)
    {
        return false;
    }

    OutSPIRVCode.resize(FileSize / sizeof(std::uint32_t), std::uint32_t());

    File.seekg(0);
    std::istream const &ReadResult = File.read(reinterpret_cast<char *>(std::data(OutSPIRVCode)), static_cast<std::streamsize>(FileSize));
    File.close();

    return !ReadResult.fail();
}

bool RenderCore::CompileOrLoadIfExists(std::string_view const      Source,
                                       ShaderType const            ShaderType,
                                       std::string_view const      EntryPoint,
                                       std::int32_t const          Version,
                                       EShLanguage const           Language,
                                       std::vector<std::uint32_t> &OutSPIRVCode)
{
    if (std::string const CompiledShaderPath = std::format("{}_{}.spv", Source, static_cast<std::uint8_t>(Language));
        std::filesystem::exists(CompiledShaderPath))
    {
        return Load(CompiledShaderPath, OutSPIRVCode);
    }

    return Compile(Source, ShaderType, EntryPoint, Version, Language, OutSPIRVCode);
}

VkShaderModule RenderCore::CreateModule(std::vector<std::uint32_t> const &SPIRVCode, EShLanguage const Language, std::string_view const EntryPoint)
{
#ifdef _DEBUG
    if (!ValidateSPIRV(SPIRVCode))
    {
        return VK_NULL_HANDLE;
    }
#endif

    VkShaderModuleCreateInfo const CreateInfo {.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                               .codeSize = std::size(SPIRVCode) * sizeof(std::uint32_t),
                                               .pCode    = std::data(SPIRVCode)};

    VkShaderModule Output = nullptr;
    CheckVulkanResult(vkCreateShaderModule(volkGetLoadedDevice(), &CreateInfo, nullptr, &Output));

    StageInfo(Output, Language, EntryPoint);

    return Output;
}

VkPipelineShaderStageCreateInfo RenderCore::GetStageInfo(VkShaderModule const &Module)
{
    return g_StageInfos.at(Module);
}

std::vector<VkShaderModule> RenderCore::GetShaderModules()
{
    std::vector<VkShaderModule> Output;
    for (auto const &ShaderModule : g_StageInfos | std::views::keys)
    {
        Output.push_back(ShaderModule);
    }

    return Output;
}

std::vector<VkPipelineShaderStageCreateInfo> RenderCore::GetStageInfos()
{
    std::vector<VkPipelineShaderStageCreateInfo> Output;
    for (auto const &StageInfo : g_StageInfos | std::views::values)
    {
        Output.push_back(StageInfo);
    }

    return Output;
}

void RenderCore::ReleaseShaderResources()
{
    for (auto const &ShaderModule : g_StageInfos | std::views::keys)
    {
        if (ShaderModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(volkGetLoadedDevice(), ShaderModule, nullptr);
        }
    }
    g_StageInfos.clear();
}

void RenderCore::FreeStagedModules(std::vector<VkPipelineShaderStageCreateInfo> const &StagedModules)
{
    for (const auto &[Type, Next, Flags, Stage, Module, Name, Info] : StagedModules)
    {
        if (Module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(volkGetLoadedDevice(), Module, nullptr);
        }

        if (g_StageInfos.contains(Module))
        {
            g_StageInfos.erase(Module);
        }
    }
}

std::vector<VkPipelineShaderStageCreateInfo> RenderCore::CompileDefaultShaders()
{
    constexpr auto GlslVersion = 450;
    constexpr auto EntryPoint  = "main";
    constexpr auto VertexHlslShader {DEFAULT_VERTEX_SHADER};
    constexpr auto FragmentHlslShader {DEFAULT_FRAGMENT_SHADER};

    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

    if (std::vector<std::uint32_t> ShaderCode; CompileOrLoadIfExists(VertexHlslShader, ShaderType::GLSL, EntryPoint, GlslVersion, EShLangVertex, ShaderCode))
    {
        auto const VertexModule = CreateModule(ShaderCode, EShLangVertex, EntryPoint);
        ShaderStages.push_back(GetStageInfo(VertexModule));
    }

    if (std::vector<std::uint32_t> ShaderCode;
        CompileOrLoadIfExists(FragmentHlslShader, ShaderType::GLSL, EntryPoint, GlslVersion, EShLangFragment, ShaderCode))
    {
        auto const FragmentModule = CreateModule(ShaderCode, EShLangFragment, EntryPoint);
        ShaderStages.push_back(GetStageInfo(FragmentModule));
    }

    return ShaderStages;
}
