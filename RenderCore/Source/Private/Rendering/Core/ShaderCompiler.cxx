// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/SPIRV/Logger.h>

#ifdef _DEBUG
#include <spirv-tools/libspirv.hpp>
#endif

module RenderCore.Runtime.ShaderCompiler;

using namespace RenderCore;

std::vector<strzilla::string> ExtractShaderIncludesInternal(strzilla::string_view const& Input)
{
    std::vector<strzilla::string> MatchingIncludes;

    std::regex includeRegex("#include\\s+\"([^\"]+)\"");

    std::string const InputStr(std::begin(Input), std::end(Input));
    std::sregex_iterator RegexIterator(std::cbegin(InputStr), std::cend(InputStr), includeRegex);

    for (std::sregex_iterator EndIterator; RegexIterator != EndIterator; ++RegexIterator)
    {
        MatchingIncludes.emplace_back((*RegexIterator)[1].str());
    }

    return MatchingIncludes;
}

class SimpleIncluder : public glslang::TShader::Includer
{
public:
    void TryInclude(strzilla::string const& HeaderName, std::filesystem::path const& Path)
    {
        if (m_Includes.contains(HeaderName) || m_Sources.contains(HeaderName))
        {
            return;
        }

        std::filesystem::path const IncludePath { Path / std::data(HeaderName) };
        std::stringstream           IncludeSource;
        std::ifstream               IncludeFile { IncludePath };

        if (!IncludeFile.is_open())
        {
            return;
        }

        IncludeSource << IncludeFile.rdbuf();
        IncludeFile.close();

        strzilla::string const SourceContent { std::data(IncludeSource.str()) };
        std::vector<strzilla::string> AdditionalIncludes = ExtractShaderIncludesInternal(std::data(SourceContent));

        for (strzilla::string_view const& IncludeIt : AdditionalIncludes)
        {
            TryInclude(IncludeIt, Path);
        }

        AddSourceZilla(HeaderName, SourceContent);
    }

    IncludeResult* includeSystem(const char* HeaderName, const char* IncluderName, std::size_t InclusionDepth) override
    {
        return nullptr;
    }

    IncludeResult* includeLocal(const char* HeaderName, const char* IncluderName, std::size_t InclusionDepth) override
    {
        strzilla::string const IncludedHeader { HeaderName };
        if (auto const SourceIt = m_Sources.find(IncludedHeader); SourceIt != std::end(m_Sources))
        {
            auto const &MatchingSource = SourceIt->second;
            auto const Result = std::make_shared<IncludeResult>(HeaderName, std::data(MatchingSource), std::size(MatchingSource), nullptr);
            m_Includes.emplace(IncludedHeader, Result);

            return Result.get();
        }

        return &s_FailResult;
    }

    void releaseInclude(IncludeResult* Include) override
    {
        if (Include != &s_FailResult)
        {
            strzilla::string const HeaderName { std::data(Include->headerName) };

            if (m_Includes.contains(HeaderName))
            {
                m_Includes.erase(HeaderName);
            }

            if (m_Sources.contains(HeaderName))
            {
                m_Sources.erase(HeaderName);
            }
        }
    }

    void addSource(const std::string& HeaderName, const std::string& Source)
    {
        AddSourceZilla(strzilla::string{ HeaderName }, strzilla::string{ std::data(Source) });
    }

    void AddSourceZilla(const strzilla::string& HeaderName, const strzilla::string& Source)
    {
        m_Sources.emplace(HeaderName, std::data(Source));
    }

private:
    static inline const strzilla::string s_Empty;
    static inline IncludeResult s_FailResult{s_Empty, "invalid header", 14, nullptr};

    std::map<strzilla::string, std::shared_ptr<IncludeResult>> m_Includes;
    std::map<strzilla::string, strzilla::string> m_Sources;
};

bool CompileInternal(ShaderType const             ShaderType,
                     strzilla::string_view const  Source,
                     std::filesystem::path const &Path,
                     EShLanguage const            Language,
                     strzilla::string_view const  EntryPoint,
                     std::int32_t const           Version,
                     std::vector<std::uint32_t>  &OutSPIRVCode)
{
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Compiling shader: " << Path;

#ifdef _DEBUG
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shader source content: " << Source;
#endif // _DEBUG

    glslang::InitializeProcess();
    glslang::TShader Shader(Language);

    auto const SourceStandardContent = std::data(Source);

    std::vector<strzilla::string> MatchingIncludes = ExtractShaderIncludesInternal(std::data(Source));

    strzilla::string const CurrentPath { std::data(Path.string()) };
    strzilla::string const CurrentFilename { std::data(Path.filename().string()) };

    MatchingIncludes.insert(std::begin(MatchingIncludes), CurrentFilename);

    std::vector<char*> Includes(std::size(MatchingIncludes), nullptr);

    for (std::size_t Iterator = 0U; Iterator < std::size(MatchingIncludes); ++Iterator)
    {
        Includes.at(Iterator) = std::data(MatchingIncludes.at(Iterator));
    }

    Shader.setStringsWithLengthsAndNames(&SourceStandardContent, nullptr, std::data(Includes), 1);

    Shader.setEntryPoint(std::data(EntryPoint));
    Shader.setEnvInputVulkanRulesRelaxed();
    Shader.setSourceEntryPoint(std::data(EntryPoint));
    Shader.setEnvInput(ShaderType == ShaderType::GLSL ? glslang::EShSourceGlsl : glslang::EShSourceHlsl, Language, glslang::EShClientVulkan, 1);
    Shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
    Shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);

    TBuiltInResource const *Resources    = GetDefaultResources();
    constexpr auto          MessageFlags = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    SimpleIncluder ShaderIncluder {};
    std::filesystem::path const ParentPath = Path.parent_path();

    for (strzilla::string const& IncludeIt : MatchingIncludes)
    {
        if (IncludeIt == CurrentFilename)
        {
            ShaderIncluder.AddSourceZilla(IncludeIt, Source);
        }
        else
        {
            ShaderIncluder.TryInclude(std::data(IncludeIt), ParentPath);
        }
    }

    if (!Shader.parse(Resources, Version, ECoreProfile, false, true, MessageFlags, ShaderIncluder))
    {
        glslang::FinalizeProcess();

        auto const InfoLog(strzilla::string { "Info Log: " } + Shader.getInfoLog());
        auto const DebugLog(strzilla::string { "Debug Log: " } + Shader.getInfoDebugLog());

        BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: " << std::format("Failed to parse shader '{}':\n{}\n{}",
                                                                            std::data(CurrentPath),
                                                                            std::data(InfoLog),
                                                                            std::data(DebugLog));
        return false;
    }

    glslang::TProgram Program;
    Program.addShader(&Shader);

    if (!Program.link(MessageFlags))
    {
        glslang::FinalizeProcess();

        auto const InfoLog(strzilla::string { "Info Log: " } + Shader.getInfoLog());
        auto const DebugLog(strzilla::string { "Debug Log: " } + Shader.getInfoDebugLog());

        BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: " << std::format("Failed to parse shader '{}':\n{}\n{}",
                                                                            std::data(CurrentPath),
                                                                            std::data(InfoLog),
                                                                            std::data(DebugLog));
        return false;
    }

    spv::SpvBuildLogger Logger;
    GlslangToSpv(*Program.getIntermediate(Language), OutSPIRVCode, &Logger);
    glslang::FinalizeProcess();

    if (strzilla::string const GeneratedLogs = Logger.getAllMessages();
        !std::empty(GeneratedLogs))
    {
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Shader compilation result log:\n" << GeneratedLogs;
    }

    return !std::empty(OutSPIRVCode);
}

#ifdef _DEBUG
bool ValidateSPIRV(std::filesystem::path const& Path, std::vector<std::uint32_t> const &SPIRVData)
{
    static spvtools::SpirvTools SPIRVToolsInstance(SPV_ENV_VULKAN_1_3);

    if (!SPIRVToolsInstance.IsValid())
    {
        return false;
    }

    static bool Initialized = false;

    if (!Initialized)
    {
        SPIRVToolsInstance.SetMessageConsumer([_func_internal_ = __func__](spv_message_level_t const              Level,
                                                                           [[maybe_unused]] char const *          Source,
                                                                           [[maybe_unused]] spv_position_t const &Position,
                                                                           char const *                           Message)
        {
            switch (Level)
            {
                case SPV_MSG_FATAL:
                case SPV_MSG_INTERNAL_ERROR:
                case SPV_MSG_ERROR: BOOST_LOG_TRIVIAL(error) << "[" << _func_internal_ << "]: " << std::format("Error: {}\n", Message);
                    break;

                case SPV_MSG_WARNING: BOOST_LOG_TRIVIAL(warning) << "[" << _func_internal_ << "]: " << std::format("Warning: {}\n", Message);
                    break;

                case SPV_MSG_INFO: BOOST_LOG_TRIVIAL(info) << "[" << _func_internal_ << "]: " << std::format("Info: {}\n", Message);
                    break;

                default:
                    break;
            }
        });

        Initialized = true;
    }

    if (std::string DisassemblySPIRVData; SPIRVToolsInstance.Disassemble(SPIRVData, &DisassemblySPIRVData))
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Generated SPIR-V shader disassembly for the shader file '" << Path << "':\n" << DisassemblySPIRVData;
    }

    return SPIRVToolsInstance.Validate(SPIRVData);
}
#endif // _DEBUG

bool RenderCore::Compile(strzilla::string_view const Source,
                         ShaderType const            ShaderType,
                         strzilla::string_view const EntryPoint,
                         std::int32_t const          Version,
                         EShLanguage const           Language,
                         std::vector<std::uint32_t> &OutSPIRVCode)
{
    std::filesystem::path const Path { std::data(Source) };
    std::stringstream           ShaderSource;
    std::ifstream               File { Path };

    if (!File.is_open())
    {
        return false;
    }

    ShaderSource << File.rdbuf();
    File.close();

    bool const Result = CompileInternal(ShaderType, ShaderSource.str(), Path, Language, EntryPoint, Version, OutSPIRVCode);

    if (Result)
    {
        #ifdef _DEBUG
        if (!ValidateSPIRV(Path, OutSPIRVCode))
        {
            return false;
        }
        #endif // _DEBUG

        strzilla::string const SPIRVPath = std::format("{}_{}.spv", std::data(Source), static_cast<std::uint8_t>(Language));
        std::ofstream          SPIRVFile(SPIRVPath, std::ios::binary);
        if (!SPIRVFile.is_open())
        {
            return false;
        }

        SPIRVFile << strzilla::string(reinterpret_cast<char const *>(std::data(OutSPIRVCode)), std::size(OutSPIRVCode) * sizeof(std::uint32_t));
        SPIRVFile.close();
    }

    return Result;
}

bool RenderCore::Load(strzilla::string_view const Source, std::vector<std::uint32_t> &OutSPIRVCode)
{
    std::filesystem::path const Path { std::data(Source) };
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

bool RenderCore::CompileOrLoadIfExists(strzilla::string_view const Source,
                                       ShaderType const            ShaderType,
                                       strzilla::string_view const EntryPoint,
                                       std::int32_t const          Version,
                                       EShLanguage const           Language,
                                       std::vector<std::uint32_t> &OutSPIRVCode)
{
    if (strzilla::string const CompiledShaderPath = std::format("{}_{}.spv", std::data(Source), static_cast<std::uint8_t>(Language));
        std::filesystem::exists(std::data(CompiledShaderPath)))
    {
        return Load(CompiledShaderPath, OutSPIRVCode);
    }

    return Compile(Source, ShaderType, EntryPoint, Version, Language, OutSPIRVCode);
}

void RenderCore::CompileDefaultShaders()
{
    constexpr auto ShaderType  = ShaderType::GLSL;
    constexpr auto GlslVersion = 450;
    constexpr auto EntryPoint  = "main";

    auto const CompileAndStage = [EntryPoint, GlslVersion] (strzilla::string_view const Shader, EShLanguage const Language, VkShaderStageFlagBits const Stage)
    {
        if (auto &[StageInfo, ShaderCode] = g_StageInfos.emplace_back();
            CompileOrLoadIfExists(Shader, ShaderType, EntryPoint, GlslVersion, Language, ShaderCode))
        {
            StageInfo = VkPipelineShaderStageCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = Stage,
                    .pName = EntryPoint
            };
        }
    };

    // Compute
    // {
    //     constexpr auto ShaderFile { DEFAULT_COMPUTE_SHADER };
    //     constexpr auto ShaderStage { EShLangCompute };
    //     constexpr auto ShaderStageFlag { VK_SHADER_STAGE_COMPUTE_BIT };
    //     CompileAndStage(ShaderFile, ShaderStage, ShaderStageFlag);
    // }

    // Task
    {
        constexpr auto ShaderFile { DEFAULT_TASK_SHADER };
        constexpr auto ShaderStage { EShLangTask };
        constexpr auto ShaderStageFlag { VK_SHADER_STAGE_TASK_BIT_EXT };
        CompileAndStage(ShaderFile, ShaderStage, ShaderStageFlag);
    }

    // Mesh
    {
        constexpr auto ShaderFile { DEFAULT_MESH_SHADER };
        constexpr auto ShaderLanguage { EShLangMesh };
        constexpr auto ShaderStageFlag { VK_SHADER_STAGE_MESH_BIT_EXT };
        CompileAndStage(ShaderFile, ShaderLanguage, ShaderStageFlag);
    }

    // Fragment
    {
        constexpr auto ShaderFile { DEFAULT_FRAGMENT_SHADER };
        constexpr auto ShaderLanguage { EShLangFragment };
        constexpr auto ShaderStageFlag { VK_SHADER_STAGE_FRAGMENT_BIT };
        CompileAndStage(ShaderFile, ShaderLanguage, ShaderStageFlag);
    }
}
