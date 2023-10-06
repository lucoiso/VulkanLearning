// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <boost/log/trivial.hpp>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <volk.h>

module RenderCore.Management.ShaderManagement;

import <unordered_map>;
import <vector>;
import <ranges>;
import <format>;
import <fstream>;
import <sstream>;
import <string>;
import <string_view>;
import <filesystem>;
import <span>;

import RenderCore.Management.DeviceManagement;
import RenderCore.Utils.Helpers;

using namespace RenderCore;

constexpr char const* g_EntryPoint         = "main";
constexpr std::int32_t const g_GlslVersion = 450;

static std::unordered_map<VkShaderModule, VkPipelineShaderStageCreateInfo> s_StageInfos {};

bool Compile(std::string_view const Source, EShLanguage const Language, std::vector<std::uint32_t>& OutSPIRVCode)
{
    glslang::InitializeProcess();

    glslang::TShader Shader(Language);

    char const* ShaderContent = Source.data();
    Shader.setStringsWithLengths(&ShaderContent, nullptr, 1);

    Shader.setEntryPoint(g_EntryPoint);
    Shader.setSourceEntryPoint(g_EntryPoint);
    Shader.setEnvInput(glslang::EShSourceGlsl, Language, glslang::EShClientVulkan, 1);
    Shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    Shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    TBuiltInResource const* Resources = GetDefaultResources();
    constexpr auto MessageFlags       = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if (!Shader.parse(Resources, g_GlslVersion, ECoreProfile, false, true, MessageFlags))
    {
        glslang::FinalizeProcess();
        std::string const InfoLog("Info Log: " + std::string(Shader.getInfoLog()));
        std::string const DebugLog("Debug Log: " + std::string(Shader.getInfoDebugLog()));
        std::string const ErrMessage = std::format("Failed to parse shader:\n{}\n{}", InfoLog, DebugLog);
        throw std::runtime_error(ErrMessage);
    }

    glslang::TProgram Program;
    Program.addShader(&Shader);

    if (!Program.link(MessageFlags))
    {
        glslang::FinalizeProcess();
        std::string const InfoLog("Info Log: " + std::string(Program.getInfoLog()));
        std::string const DebugLog("Debug Log: " + std::string(Program.getInfoDebugLog()));
        std::string const ErrMessage = std::format("Failed to parse shader:\n{}\n{}", InfoLog, DebugLog);
        throw std::runtime_error(ErrMessage);
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Compiling shader:\n"
                             << Source;

    spv::SpvBuildLogger Logger;
    GlslangToSpv(*Program.getIntermediate(Language), OutSPIRVCode, &Logger);
    glslang::FinalizeProcess();

    if (std::string const GeneratedLogs = Logger.getAllMessages();
        !GeneratedLogs.empty())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shader compilation result log:\n"
                                 << GeneratedLogs;
    }

    return !OutSPIRVCode.empty();
}

void StageInfo(VkShaderModule const& Module, EShLanguage const Language)
{
    if (Module == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Invalid shader module");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Staging shader info...";

    VkPipelineShaderStageCreateInfo StageInfo {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = Module,
            .pName  = g_EntryPoint};

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
            throw std::runtime_error("Unsupported shader language");
    }

    s_StageInfos.emplace(Module, StageInfo);
}

bool RenderCore::Compile(std::string_view const Source, std::vector<uint32_t>& OutSPIRVCode)
{
    EShLanguage Language = EShLangVertex;
    std::filesystem::path const Path(Source);

    if (std::filesystem::path const Extension = Path.extension();
        Extension == ".frag")
    {
        Language = EShLangFragment;
    }
    else if (Extension == ".comp")
    {
        Language = EShLangCompute;
    }
    else if (Extension == ".geom")
    {
        Language = EShLangGeometry;
    }
    else if (Extension == ".tesc")
    {
        Language = EShLangTessControl;
    }
    else if (Extension == ".tese")
    {
        Language = EShLangTessEvaluation;
    }
    else if (Extension == ".rgen")
    {
        Language = EShLangRayGen;
    }
    else if (Extension == ".rint")
    {
        Language = EShLangIntersect;
    }
    else if (Extension == ".rahit")
    {
        Language = EShLangAnyHit;
    }
    else if (Extension == ".rchit")
    {
        Language = EShLangClosestHit;
    }
    else if (Extension == ".rmiss")
    {
        Language = EShLangMiss;
    }
    else if (Extension == ".rcall")
    {
        Language = EShLangCallable;
    }
    else if (Extension != ".vert")
    {
        throw std::runtime_error("Unknown shader extension: " + Extension.string());
    }

    std::stringstream ShaderSource;
    std::ifstream File(Path);
    if (!File.is_open())
    {
        throw std::runtime_error("Failed to open shader file: " + Path.string());
    }

    ShaderSource << File.rdbuf();
    File.close();

    bool const Result = Compile(ShaderSource.str(), Language, OutSPIRVCode);
    if (Result)
    {
        std::string const SPIRVPath = std::format("{}.spv", Source);
        std::ofstream SPIRVFile(SPIRVPath, std::ios::binary);
        if (!SPIRVFile.is_open())
        {
            throw std::runtime_error("Failed to open SPIRV file: " + SPIRVPath);
        }

        SPIRVFile << std::string(reinterpret_cast<char const*>(OutSPIRVCode.data()), OutSPIRVCode.size() * sizeof(uint32_t));
        SPIRVFile.close();

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shader compiled, generated SPIR-V shader file: " << SPIRVPath;
    }

    return Result;
}

bool RenderCore::Load(std::string_view const Source, std::vector<std::uint32_t>& OutSPIRVCode)
{
    std::filesystem::path const Path(Source);
    if (!exists(Path))
    {
        std::string const ErrMessage = "Shader file does not exist" + std::string(Source);
        throw std::runtime_error(ErrMessage);
    }

    std::ifstream File(Path, std::ios::ate | std::ios::binary);
    if (!File.is_open())
    {
        std::string const ErrMessage = "Failed to open shader file" + std::string(Source);
        throw std::runtime_error(ErrMessage);
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading shader: " << Source;

    size_t const FileSize = File.tellg();

    if (FileSize == 0)
    {
        throw std::runtime_error("Shader file is empty");
    }

    OutSPIRVCode.resize(FileSize / sizeof(std::uint32_t), std::uint32_t());

    File.seekg(0);
    std::istream const& ReadResult = File.read(reinterpret_cast<char*>(OutSPIRVCode.data()), static_cast<std::streamsize>(FileSize)); /* Flawfinder: ignore */
    File.close();

    return !ReadResult.fail();
}

bool RenderCore::CompileOrLoadIfExists(std::string_view const Source, std::vector<uint32_t>& OutSPIRVCode)
{
    if (std::string const CompiledShaderPath = std::format("{}.spv", Source);
        std::filesystem::exists(CompiledShaderPath))
    {
        return Load(CompiledShaderPath, OutSPIRVCode);
    }
    return Compile(Source, OutSPIRVCode);
}

VkShaderModule RenderCore::CreateModule(VkDevice const& Device, std::vector<std::uint32_t> const& SPIRVCode, EShLanguage const Language)
{
    if (Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Invalid vulkan logical device");
    }

    if (SPIRVCode.empty())
    {
        throw std::runtime_error("Invalid SPIRV code");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating shader module...";

    VkShaderModuleCreateInfo const CreateInfo {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = SPIRVCode.size() * sizeof(std::uint32_t),
            .pCode    = SPIRVCode.data()};

    VkShaderModule Output = nullptr;
    CheckVulkanResult(vkCreateShaderModule(Device, &CreateInfo, nullptr, &Output));

    StageInfo(Output, Language);

    return Output;
}

VkPipelineShaderStageCreateInfo RenderCore::GetStageInfo(VkShaderModule const& Module)
{
    return s_StageInfos.at(Module);
}

std::vector<VkShaderModule> RenderCore::GetShaderModules()
{
    std::vector<VkShaderModule> Output;
    for (auto const& ShaderModule: s_StageInfos | std::views::keys)
    {
        Output.push_back(ShaderModule);
    }

    return Output;
}

std::vector<VkPipelineShaderStageCreateInfo> RenderCore::GetStageInfos()
{
    std::vector<VkPipelineShaderStageCreateInfo> Output;
    for (auto const& StageInfo: s_StageInfos | std::views::values)
    {
        Output.push_back(StageInfo);
    }

    return Output;
}

void RenderCore::ReleaseShaderResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan shader resources";

    VkDevice const& VulkanLogicalDevice = GetLogicalDevice();

    for (auto const& ShaderModule: s_StageInfos | std::views::keys)
    {
        if (ShaderModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(VulkanLogicalDevice, ShaderModule, nullptr);
        }
    }
    s_StageInfos.clear();
}

void RenderCore::FreeStagedModules(std::vector<VkPipelineShaderStageCreateInfo> const& StagedModules)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Freeing staged shader modules";

    VkDevice const& VulkanLogicalDevice = GetLogicalDevice();

    for (VkPipelineShaderStageCreateInfo const& StageInfoIter: StagedModules)
    {
        if (StageInfoIter.module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(VulkanLogicalDevice, StageInfoIter.module, nullptr);
        }

        if (s_StageInfos.contains(StageInfoIter.module))
        {
            s_StageInfos.erase(StageInfoIter.module);
        }
    }
}