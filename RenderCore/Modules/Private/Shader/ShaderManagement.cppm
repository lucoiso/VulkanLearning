// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <filesystem>
#include <fstream>
#include <ranges>
#include <volk.h>
#include <boost/log/trivial.hpp>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spirv-tools/libspirv.hpp>

module RenderCore.Management.ShaderManagement;

import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RuntimeInfo.Manager;

using namespace RenderCore;

constexpr char const *g_EntryPoint = "main";
constexpr std::int32_t g_GlslVersion = 450;

std::unordered_map<VkShaderModule, VkPipelineShaderStageCreateInfo> g_StageInfos{};

bool Compile(std::string_view const &Source, EShLanguage const Language, std::vector<std::uint32_t> &OutSPIRVCode)
{
    auto const _ { RuntimeInfo::Manager::Get().PushCallstackWithCounter() };

    glslang::InitializeProcess();

    glslang::TShader Shader(Language);

    char const *ShaderContent = std::data(Source);
    Shader.setStringsWithLengths(&ShaderContent, nullptr, 1);

    Shader.setEntryPoint(g_EntryPoint);
    Shader.setSourceEntryPoint(g_EntryPoint);
    Shader.setEnvInput(glslang::EShSourceGlsl, Language, glslang::EShClientVulkan, 1);
    Shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    Shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    TBuiltInResource const *Resources = GetDefaultResources();
    constexpr auto MessageFlags = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if (!Shader.parse(Resources, g_GlslVersion, ECoreProfile, false, true, MessageFlags))
    {
        glslang::FinalizeProcess();
        std::string const InfoLog("Info Log: " + std::string(Shader.getInfoLog()));
        std::string const DebugLog("Debug Log: " + std::string(Shader.getInfoDebugLog()));
        throw std::runtime_error(std::format("Failed to parse shader:\n{}\n{}", InfoLog, DebugLog));
    }

    glslang::TProgram Program;
    Program.addShader(&Shader);

    if (!Program.link(MessageFlags))
    {
        glslang::FinalizeProcess();
        std::string const InfoLog("Info Log: " + std::string(Program.getInfoLog()));
        std::string const DebugLog("Debug Log: " + std::string(Program.getInfoDebugLog()));
        throw std::runtime_error(std::format("Failed to parse shader:\n{}\n{}", InfoLog, DebugLog));
    }

    if constexpr (g_EnableCustomDebug)
    {
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Compiling shader:\n"
                                 << Source;
    }

    spv::SpvBuildLogger Logger;
    GlslangToSpv(*Program.getIntermediate(Language), OutSPIRVCode, &Logger);
    glslang::FinalizeProcess();

    if (std::string const GeneratedLogs = Logger.getAllMessages();
        !std::empty(GeneratedLogs))
    {
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Shader compilation result log:\n"
                                 << GeneratedLogs;
    }

    return !std::empty(OutSPIRVCode);
}

void StageInfo(VkShaderModule const &Module, EShLanguage const Language)
{
    auto const _ { RuntimeInfo::Manager::Get().PushCallstackWithCounter() };
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Staging shader info...";

    if (Module == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Invalid shader module");
    }

    VkPipelineShaderStageCreateInfo StageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module = Module,
        .pName = g_EntryPoint
    };

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

    g_StageInfos.emplace(Module, StageInfo);
}

#ifdef _DEBUG
bool ValidateSPIRV(const std::vector<std::uint32_t> &SPIRVData)
{
    static spvtools::SpirvTools SPIRVToolsInstance(SPV_ENV_VULKAN_1_3);
    if (!SPIRVToolsInstance.IsValid())
    {
        throw std::runtime_error("Failed to initialize SPIRV-Tools");
    }

    if constexpr (g_EnableCustomDebug)
    {
        if (std::string DisassemblySPIRVData;
            SPIRVToolsInstance.Disassemble(SPIRVData, &DisassemblySPIRVData))
        {
            BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Generated SPIR-V shader disassembly:\n"
                                     << DisassemblySPIRVData;
        }
    }

    return SPIRVToolsInstance.Validate(SPIRVData);
}
#endif

bool RenderCore::Compile(std::string_view const &Source, std::vector<std::uint32_t> &OutSPIRVCode)
{
    auto const _ { RuntimeInfo::Manager::Get().PushCallstackWithCounter() };

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
        throw std::runtime_error(std::format("Unknown shader extension: {}", Extension.string()));
    }

    std::stringstream ShaderSource;
    std::ifstream File(Path);
    if (!File.is_open())
    {
        throw std::runtime_error(std::format("Failed to open shader file: {}", Path.string()));
    }

    ShaderSource << File.rdbuf();
    File.close();

    bool const Result = Compile(ShaderSource.str(), Language, OutSPIRVCode);
    if (Result)
    {
#ifdef _DEBUG
        if (!ValidateSPIRV(OutSPIRVCode))
        {
            throw std::runtime_error("Failed to validate SPIR-V code");
        }
#endif

        std::string const SPIRVPath = std::format("{}.spv", Source);
        std::ofstream SPIRVFile(SPIRVPath, std::ios::binary);
        if (!SPIRVFile.is_open())
        {
            throw std::runtime_error(std::format("Failed to open SPIRV file: {}", SPIRVPath));
        }

        SPIRVFile << std::string(reinterpret_cast<char const *>(std::data(OutSPIRVCode)), std::size(OutSPIRVCode) * sizeof(std::uint32_t));
        SPIRVFile.close();

        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Shader compiled, generated SPIR-V shader file: " << SPIRVPath;
    }

    return Result;
}

bool RenderCore::Load(std::string_view const &Source, std::vector<std::uint32_t> &OutSPIRVCode)
{
    auto const _ { RuntimeInfo::Manager::Get().PushCallstackWithCounter() };
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Loading shader: " << Source;

    std::filesystem::path const Path(Source);
    if (!exists(Path))
    {
        throw std::runtime_error(std::format("Shader file does not exist {}", Source));
    }

    std::ifstream File(Path, std::ios::ate | std::ios::binary);
    if (!File.is_open())
    {
        throw std::runtime_error(std::format("Failed to open shader file {}", Source));
    }

    std::size_t const FileSize = File.tellg();
    if (FileSize == 0)
    {
        throw std::runtime_error("Shader file is empty");
    }

    OutSPIRVCode.resize(FileSize / sizeof(std::uint32_t), std::uint32_t());

    File.seekg(0);
    std::istream const &ReadResult = File.read(reinterpret_cast<char *>(std::data(OutSPIRVCode)), static_cast<std::streamsize>(FileSize));
    /* Flawfinder: ignore */
    File.close();

    return !ReadResult.fail();
}

bool RenderCore::CompileOrLoadIfExists(std::string_view const &Source, std::vector<uint32_t> &OutSPIRVCode)
{
    auto const _ { RuntimeInfo::Manager::Get().PushCallstackWithCounter() };

    if (std::string const CompiledShaderPath = std::format("{}.spv", Source);
        std::filesystem::exists(CompiledShaderPath))
    {
        return Load(CompiledShaderPath, OutSPIRVCode);
    }
    return Compile(Source, OutSPIRVCode);
}

VkShaderModule RenderCore::CreateModule(std::vector<std::uint32_t> const &SPIRVCode, EShLanguage const Language)
{
    auto const _ { RuntimeInfo::Manager::Get().PushCallstackWithCounter() };
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating shader module";

    if (std::empty(SPIRVCode))
    {
        throw std::runtime_error("Invalid SPIRV code");
    }

#ifdef _DEBUG
    if (!ValidateSPIRV(SPIRVCode))
    {
        throw std::runtime_error("Failed to validate SPIR-V code");
    }
#endif

    VkShaderModuleCreateInfo const CreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = std::size(SPIRVCode) * sizeof(std::uint32_t),
        .pCode = std::data(SPIRVCode)
    };

    VkShaderModule Output = nullptr;
    CheckVulkanResult(vkCreateShaderModule(volkGetLoadedDevice(), &CreateInfo, nullptr, &Output));

    StageInfo(Output, Language);

    return Output;
}

VkPipelineShaderStageCreateInfo RenderCore::GetStageInfo(VkShaderModule const &Module)
{
    return g_StageInfos.at(Module);
}

std::vector<VkShaderModule> RenderCore::GetShaderModules()
{
    std::vector<VkShaderModule> Output;
    for (auto const &ShaderModule: g_StageInfos | std::views::keys)
    {
        Output.push_back(ShaderModule);
    }

    return Output;
}

std::vector<VkPipelineShaderStageCreateInfo> RenderCore::GetStageInfos()
{
    std::vector<VkPipelineShaderStageCreateInfo> Output;
    for (auto const &StageInfo: g_StageInfos | std::views::values)
    {
        Output.push_back(StageInfo);
    }

    return Output;
}

void RenderCore::ReleaseShaderResources()
{
    auto const _ { RuntimeInfo::Manager::Get().PushCallstackWithCounter() };
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Releasing vulkan shader resources";

    for (auto const &ShaderModule: g_StageInfos | std::views::keys)
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
    auto const _ { RuntimeInfo::Manager::Get().PushCallstackWithCounter() };
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Freeing staged shader modules";

    for (VkPipelineShaderStageCreateInfo const &StageInfoIter: StagedModules)
    {
        if (StageInfoIter.module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(volkGetLoadedDevice(), StageInfoIter.module, nullptr);
        }

        if (g_StageInfos.contains(StageInfoIter.module))
        {
            g_StageInfos.erase(StageInfoIter.module);
        }
    }
}

std::vector<VkPipelineShaderStageCreateInfo> RenderCore::CompileDefaultShaders()
{
    constexpr std::array FragmentShaders{
        DEFAULT_SHADER_FRAG
    };
    constexpr std::array VertexShaders{
        DEFAULT_SHADER_VERT
    };

    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

    for (char const *const&FragmentShaderIter: FragmentShaders)
    {
        if (std::vector<std::uint32_t> FragmentShaderCode;
            CompileOrLoadIfExists(FragmentShaderIter, FragmentShaderCode))
        {
            auto const FragmentModule = CreateModule(FragmentShaderCode, EShLangFragment);
            ShaderStages.push_back(GetStageInfo(FragmentModule));
        }
    }

    for (char const *const&VertexShaderIter: VertexShaders)
    {
        if (std::vector<std::uint32_t> VertexShaderCode;
            CompileOrLoadIfExists(VertexShaderIter, VertexShaderCode))
        {
            auto const VertexModule = CreateModule(VertexShaderCode, EShLangVertex);
            ShaderStages.push_back(GetStageInfo(VertexModule));
        }
    }

    return ShaderStages;
}
