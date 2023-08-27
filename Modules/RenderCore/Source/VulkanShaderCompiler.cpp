// Copyright Notice: [...]

#include "VulkanShaderCompiler.h"
#include <boost/log/trivial.hpp>
#include <SPIRV/GlslangToSpv.h>
#include <filesystem>
#include <fstream>

constexpr TBuiltInResource InitResources()
{
    TBuiltInResource Resources;

    Resources.maxLights = 32;
    Resources.maxClipPlanes = 6;
    Resources.maxTextureUnits = 32;
    Resources.maxTextureCoords = 32;
    Resources.maxVertexAttribs = 64;
    Resources.maxVertexUniformComponents = 4096;
    Resources.maxVaryingFloats = 64;
    Resources.maxVertexTextureImageUnits = 32;
    Resources.maxCombinedTextureImageUnits = 80;
    Resources.maxTextureImageUnits = 32;
    Resources.maxFragmentUniformComponents = 4096;
    Resources.maxDrawBuffers = 32;
    Resources.maxVertexUniformVectors = 128;
    Resources.maxVaryingVectors = 8;
    Resources.maxFragmentUniformVectors = 16;
    Resources.maxVertexOutputVectors = 16;
    Resources.maxFragmentInputVectors = 15;
    Resources.minProgramTexelOffset = -8;
    Resources.maxProgramTexelOffset = 7;
    Resources.maxClipDistances = 8;
    Resources.maxComputeWorkGroupCountX = 65535;
    Resources.maxComputeWorkGroupCountY = 65535;
    Resources.maxComputeWorkGroupCountZ = 65535;
    Resources.maxComputeWorkGroupSizeX = 1024;
    Resources.maxComputeWorkGroupSizeY = 1024;
    Resources.maxComputeWorkGroupSizeZ = 64;
    Resources.maxComputeUniformComponents = 1024;
    Resources.maxComputeTextureImageUnits = 16;
    Resources.maxComputeImageUniforms = 8;
    Resources.maxComputeAtomicCounters = 8;
    Resources.maxComputeAtomicCounterBuffers = 1;
    Resources.maxVaryingComponents = 60;
    Resources.maxVertexOutputComponents = 64;
    Resources.maxGeometryInputComponents = 64;
    Resources.maxGeometryOutputComponents = 128;
    Resources.maxFragmentInputComponents = 128;
    Resources.maxImageUnits = 8;
    Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
    Resources.maxCombinedShaderOutputResources = 8;
    Resources.maxImageSamples = 0;
    Resources.maxVertexImageUniforms = 0;
    Resources.maxTessControlImageUniforms = 0;
    Resources.maxTessEvaluationImageUniforms = 0;
    Resources.maxGeometryImageUniforms = 0;
    Resources.maxFragmentImageUniforms = 8;
    Resources.maxCombinedImageUniforms = 8;
    Resources.maxGeometryTextureImageUnits = 16;
    Resources.maxGeometryOutputVertices = 256;
    Resources.maxGeometryTotalOutputComponents = 1024;
    Resources.maxGeometryUniformComponents = 1024;
    Resources.maxGeometryVaryingComponents = 64;
    Resources.maxTessControlInputComponents = 128;
    Resources.maxTessControlOutputComponents = 128;
    Resources.maxTessControlTextureImageUnits = 16;
    Resources.maxTessControlUniformComponents = 1024;
    Resources.maxTessControlTotalOutputComponents = 4096;
    Resources.maxTessEvaluationInputComponents = 128;
    Resources.maxTessEvaluationOutputComponents = 128;
    Resources.maxTessEvaluationTextureImageUnits = 16;
    Resources.maxTessEvaluationUniformComponents = 1024;
    Resources.maxTessPatchComponents = 120;
    Resources.maxPatchVertices = 32;
    Resources.maxTessGenLevel = 64;
    Resources.maxViewports = 16;
    Resources.maxVertexAtomicCounters = 0;
    Resources.maxTessControlAtomicCounters = 0;
    Resources.maxTessEvaluationAtomicCounters = 0;
    Resources.maxGeometryAtomicCounters = 0;
    Resources.maxFragmentAtomicCounters = 8;
    Resources.maxCombinedAtomicCounters = 8;
    Resources.maxAtomicCounterBindings = 1;
    Resources.maxVertexAtomicCounterBuffers = 0;
    Resources.maxTessControlAtomicCounterBuffers = 0;
    Resources.maxTessEvaluationAtomicCounterBuffers = 0;
    Resources.maxGeometryAtomicCounterBuffers = 0;
    Resources.maxFragmentAtomicCounterBuffers = 1;
    Resources.maxCombinedAtomicCounterBuffers = 1;
    Resources.maxAtomicCounterBufferSize = 16384;
    Resources.maxTransformFeedbackBuffers = 4;
    Resources.maxTransformFeedbackInterleavedComponents = 64;
    Resources.maxCullDistances = 8;
    Resources.maxCombinedClipAndCullDistances = 8;
    Resources.maxSamples = 4;
    Resources.maxMeshOutputVerticesNV = 256;
    Resources.maxMeshOutputPrimitivesNV = 512;
    Resources.maxMeshWorkGroupSizeX_NV = 32;
    Resources.maxMeshWorkGroupSizeY_NV = 1;
    Resources.maxMeshWorkGroupSizeZ_NV = 1;
    Resources.maxTaskWorkGroupSizeX_NV = 32;
    Resources.maxTaskWorkGroupSizeY_NV = 1;
    Resources.maxTaskWorkGroupSizeZ_NV = 1;
    Resources.maxMeshViewCountNV = 4;

    Resources.limits.nonInductiveForLoops = 1;
    Resources.limits.whileLoops = 1;
    Resources.limits.doWhileLoops = 1;
    Resources.limits.generalUniformIndexing = 1;
    Resources.limits.generalAttributeMatrixVectorIndexing = 1;
    Resources.limits.generalVaryingIndexing = 1;
    Resources.limits.generalSamplerIndexing = 1;
    Resources.limits.generalVariableIndexing = 1;
    Resources.limits.generalConstantMatrixVectorIndexing = 1;

    return Resources;
}

using namespace RenderCore;

VulkanShaderCompiler::VulkanShaderCompiler()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan shader compiler";
}

VulkanShaderCompiler::~VulkanShaderCompiler()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan shader compiler";
}

bool VulkanShaderCompiler::CompileShader(const char* ShaderSource, std::vector<uint32_t>& OutSPIRVCode)
{
    EShLanguage ShaderLanguage = EShLangVertex;
    const std::filesystem::path ShaderPath(ShaderSource);
    
    if (const std::filesystem::path ShaderExtension = ShaderPath.extension(); 
        ShaderExtension == ".frag")
    {
        ShaderLanguage = EShLangFragment;
    }
    else if (ShaderExtension == ".comp")
    {
        ShaderLanguage = EShLangCompute;
    }
    else if (ShaderExtension == ".geom")
    {
        ShaderLanguage = EShLangGeometry;
    }
    else if (ShaderExtension == ".tesc")
    {
        ShaderLanguage = EShLangTessControl;
    }
    else if (ShaderExtension == ".tese")
    {
        ShaderLanguage = EShLangTessEvaluation;
    }
    else if (ShaderExtension == ".rgen")
    {
        ShaderLanguage = EShLangRayGen;
    }
    else if (ShaderExtension == ".rint")
    {
        ShaderLanguage = EShLangIntersect;
    }
    else if (ShaderExtension == ".rahit")
    {
        ShaderLanguage = EShLangAnyHit;
    }
    else if (ShaderExtension == ".rchit")
    {
        ShaderLanguage = EShLangClosestHit;
    }
    else if (ShaderExtension == ".rmiss")
    {
        ShaderLanguage = EShLangMiss;
    }
    else if (ShaderExtension == ".rcall")
    {
        ShaderLanguage = EShLangCallable;
    }
    else if (ShaderExtension != ".vert")
    {
        throw std::runtime_error("Unknown shader extension: " + ShaderExtension.string());
    }

    return CompileShader(ShaderSource, ShaderLanguage, OutSPIRVCode);
}

bool VulkanShaderCompiler::LoadShader(const char* ShaderSource, std::vector<uint32_t>& OutSPIRVCode)
{
    std::filesystem::path ShaderPath(ShaderSource);
    if (!std::filesystem::exists(ShaderPath))
    {
        const std::string ErrMessage = "Shader file does not exist" + std::string(ShaderSource);
        throw std::runtime_error(ErrMessage);
    }

    std::ifstream ShaderFile(ShaderPath, std::ios::ate | std::ios::binary);
    if (!ShaderFile.is_open())
    {
        const std::string ErrMessage = "Failed to open shader file" + std::string(ShaderSource);
        throw std::runtime_error(ErrMessage);
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading shader: " << ShaderSource;

    const size_t FileSize = static_cast<size_t>(ShaderFile.tellg());
    OutSPIRVCode.resize(FileSize / sizeof(uint32_t));

    ShaderFile.seekg(0);
    ShaderFile.read(reinterpret_cast<char*>(OutSPIRVCode.data()), FileSize);
    ShaderFile.close();

    return !OutSPIRVCode.empty();
}


VkShaderModule VulkanShaderCompiler::CreateShaderModule(const VkDevice& Device, const std::vector<uint32_t>& SPIRVCode, EShLanguage ShaderLanguage)
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

    StageShaderInfo(Output, ShaderLanguage);

    return Output;
}

bool VulkanShaderCompiler::CompileShader(const char* ShaderSource, EShLanguage ShaderLanguage, std::vector<uint32_t>& OutSPIRVCode)
{
    glslang::InitializeProcess();

    glslang::TShader Shader(ShaderLanguage);
    Shader.setStrings(&ShaderSource, 1);
    Shader.setPreamble("#version 450");

    Shader.setEntryPoint(EntryPoint);
    Shader.setSourceEntryPoint(EntryPoint);

    const TBuiltInResource Resources = InitResources();

    if (!Shader.parse(&Resources, 450, true, EShMessages::EShMsgEnhanced))
    {
        throw std::runtime_error("Failed to parse shader");
    }

    glslang::TProgram Program;
    Program.addShader(&Shader);

    if (!Program.link(EShMessages::EShMsgEnhanced))
    {
        const std::string ErrMessage = "Failed to link shader" + std::string(Program.getInfoLog());
        throw std::runtime_error(ErrMessage);
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Compiling shader: " << ShaderSource;

    glslang::SpvOptions Options;
#ifdef _DEBUG
    Options.generateDebugInfo = true;
#endif

    glslang::GlslangToSpv(*Program.getIntermediate(ShaderLanguage), OutSPIRVCode, &Options);
    glslang::FinalizeProcess();

    return !OutSPIRVCode.empty();
}

void VulkanShaderCompiler::StageShaderInfo(const VkShaderModule& Module, EShLanguage ShaderLanguage)
{
    if (Module == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Invalid shader module");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Staging shader info";
    VkPipelineShaderStageCreateInfo ShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module = Module,
        .pName = EntryPoint
    };

    switch (ShaderLanguage)
    {
        case EShLangVertex:
            ShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case EShLangFragment:
            ShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;

        default: throw std::runtime_error("Unsupported shader language");
    }

    m_ShaderStageInfos.emplace(Module, ShaderStageInfo);
}
