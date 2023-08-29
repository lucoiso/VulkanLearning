// Copyright Notice: [...]

#include "VulkanShaderCompiler.h"
#include <boost/log/trivial.hpp>
#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include <filesystem>
#include <fstream>
#include <format>

using namespace RenderCore;

VulkanShaderCompiler::VulkanShaderCompiler()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan shader compiler";
}

VulkanShaderCompiler::~VulkanShaderCompiler()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan shader compiler";
}

bool VulkanShaderCompiler::Compile(const std::string_view Source, std::vector<uint32_t>& OutSPIRVCode)
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

bool VulkanShaderCompiler::Load(const std::string_view Source, std::vector<uint32_t>& OutSPIRVCode)
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
    OutSPIRVCode.resize(FileSize / sizeof(uint32_t));

    File.seekg(0);
    File.read(reinterpret_cast<char*>(OutSPIRVCode.data()), FileSize);
    File.close();

    return !OutSPIRVCode.empty();
}


VkShaderModule VulkanShaderCompiler::CreateModule(const VkDevice& Device, const std::vector<uint32_t>& SPIRVCode, EShLanguage Language)
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
        .codeSize = SPIRVCode.size(),
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

bool VulkanShaderCompiler::Compile(const std::string_view Source, EShLanguage Language, std::vector<uint32_t>& OutSPIRVCode)
{
    glslang::InitializeProcess();

    glslang::TShader Shader(Language);

    const char* ShaderContent = Source.data();
    Shader.setStrings(&ShaderContent, 1);

    Shader.setEntryPoint(EntryPoint);
    Shader.setSourceEntryPoint(EntryPoint);

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

    glslang::SpvOptions Options;
#ifdef _DEBUG
    Options.generateDebugInfo = true;
#endif

    glslang::GlslangToSpv(*Program.getIntermediate(Language), OutSPIRVCode, &Options);
    glslang::FinalizeProcess();

    return !OutSPIRVCode.empty();
}

void VulkanShaderCompiler::StageInfo(const VkShaderModule& Module, EShLanguage Language)
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
