// Copyright Notice: [...]

#include "Managers/VulkanShaderManager.h"
#include <boost/log/trivial.hpp>
#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include <filesystem>
#include <fstream>
#include <format>

using namespace RenderCore;

VulkanShaderManager::VulkanShaderManager(const VkDevice& Device)
    : m_Device(Device)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan shader compiler";
}

VulkanShaderManager::~VulkanShaderManager()
{
    if (m_Device == VK_NULL_HANDLE || m_StageInfos.empty())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan shader compiler";
    Shutdown();
}

void VulkanShaderManager::Shutdown()
{
    if (m_Device == VK_NULL_HANDLE || m_StageInfos.empty())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan shader compiler";

    for (auto& [ShaderModule, _] : m_StageInfos)
    {
        if (ShaderModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_Device, ShaderModule, nullptr);
        }
    }
    m_StageInfos.clear();
}

bool VulkanShaderManager::Compile(const std::string_view Source, std::vector<uint32_t>& OutSPIRVCode)
{
    EShLanguage Language = EShLangVertex;
    const std::filesystem::path Path(Source);
    
    if (const std::filesystem::path Extension = Path.extension();
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

    const bool bResult = Compile(ShaderSource.str(), Language, OutSPIRVCode);
    if (bResult)
    {
        const std::string SPIRVPath = std::format("{}.spv", Source);
        std::ofstream SPIRVFile(SPIRVPath, std::ios::binary);
        if (!SPIRVFile.is_open())
        {
            throw std::runtime_error("Failed to open SPIRV file: " + SPIRVPath);
        }

        SPIRVFile << std::string(reinterpret_cast<const char*>(OutSPIRVCode.data()), OutSPIRVCode.size() * sizeof(uint32_t));
        SPIRVFile.close();

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shader compiled, generated SPIR-V shader file: " << SPIRVPath;
    }

    return bResult;
}

bool VulkanShaderManager::Load(const std::string_view Source, std::vector<std::uint32_t>& OutSPIRVCode)
{
    std::filesystem::path Path(Source);
    if (!std::filesystem::exists(Path))
    {
        const std::string ErrMessage = "Shader file does not exist" + std::string(Source);
        throw std::runtime_error(ErrMessage);
    }

    std::ifstream File(Path, std::ios::ate | std::ios::binary);
    if (!File.is_open())
    {
        const std::string ErrMessage = "Failed to open shader file" + std::string(Source);
        throw std::runtime_error(ErrMessage);
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading shader: " << Source;

    const size_t FileSize = static_cast<size_t>(File.tellg());

    if (FileSize == 0)
    {
        throw std::runtime_error("Shader file is empty");
    }

    OutSPIRVCode.resize(FileSize / sizeof(uint32_t));

    File.seekg(0);
    const std::istream& ReadResult = File.read(reinterpret_cast<char*>(OutSPIRVCode.data()), FileSize); /* Flawfinder: ignore */
    File.close();

    return !ReadResult.fail();
}


VkShaderModule VulkanShaderManager::CreateModule(const VkDevice& Device, const std::vector<std::uint32_t>& SPIRVCode, EShLanguage Language)
{
    if (Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Invalid vulkan logical device");
    }

    if (SPIRVCode.empty())
    {
        throw std::runtime_error("Invalid SPIRV code");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating shader module";

    const VkShaderModuleCreateInfo CreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = SPIRVCode.size() * sizeof(std::uint32_t),
        .pCode = SPIRVCode.data()
    };

    VkShaderModule Output;
    if (vkCreateShaderModule(Device, &CreateInfo, nullptr, &Output) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module");
    }

    StageInfo(Output, Language);

    return Output;
}

VkPipelineShaderStageCreateInfo VulkanShaderManager::GetStageInfo(const VkShaderModule& Module)
{
    return m_StageInfos.at(Module);
}

bool VulkanShaderManager::Compile(const std::string_view Source, EShLanguage Language, std::vector<std::uint32_t>& OutSPIRVCode)
{
    glslang::InitializeProcess();

    glslang::TShader Shader(Language);

    const char* ShaderContent = Source.data();
    Shader.setStringsWithLengths(&ShaderContent, nullptr, 1);

    Shader.setEntryPoint(EntryPoint);
    Shader.setSourceEntryPoint(EntryPoint);
    Shader.setEnvInput(glslang::EShSourceGlsl, Language, glslang::EShClientVulkan, 1);
    Shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    Shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    const TBuiltInResource* Resources = GetDefaultResources();
    const EShMessages MessageFlags = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if (!Shader.parse(Resources, 450, EProfile::ECoreProfile, false, true, MessageFlags))
    {
        glslang::FinalizeProcess();
        const std::string InfoLog("Info Log: " + std::string(Shader.getInfoLog()));
        const std::string DebugLog("Debug Log: " + std::string(Shader.getInfoDebugLog()));
        const std::string ErrMessage = std::format("Failed to parse shader:\n{}\n{}", InfoLog, DebugLog);
        throw std::runtime_error(ErrMessage);
    }

    glslang::TProgram Program;
    Program.addShader(&Shader);

    if (!Program.link(MessageFlags))
    {
        glslang::FinalizeProcess();
        const std::string InfoLog("Info Log: " + std::string(Program.getInfoLog()));
        const std::string DebugLog("Debug Log: " + std::string(Program.getInfoDebugLog()));
        const std::string ErrMessage = std::format("Failed to parse shader:\n{}\n{}", InfoLog, DebugLog);
        throw std::runtime_error(ErrMessage);
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Compiling shader:\n" << Source;
    
    spv::SpvBuildLogger Logger;
    glslang::GlslangToSpv(*Program.getIntermediate(Language), OutSPIRVCode, &Logger);
    glslang::FinalizeProcess();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Compile result log:\n" << Logger.getAllMessages();

    return !OutSPIRVCode.empty();
}

void VulkanShaderManager::StageInfo(const VkShaderModule& Module, EShLanguage Language)
{
    if (Module == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Invalid shader module");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Staging shader info";
    VkPipelineShaderStageCreateInfo StageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module = Module,
        .pName = EntryPoint
    };

    switch (Language)
    {
        case EShLangVertex:
            StageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case EShLangFragment:
            StageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;

        default: throw std::runtime_error("Unsupported shader language");
    }

    m_StageInfos.emplace(Module, StageInfo);
}
