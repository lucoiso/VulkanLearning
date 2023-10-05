// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <boost/log/trivial.hpp>
#include <glm/ext.hpp>
#include <imgui.h>

#ifndef VOLK_IMPLEMENTATION
#define VOLK_IMPLEMENTATION
#endif
#include <volk.h>

#ifdef GLFW_INCLUDE_VULKAN
#undef GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

module RenderCore.EngineCore;

import <array>;
import <cstdint>;
import <filesystem>;
import <optional>;
import <stdexcept>;
import <string_view>;
import <vector>;

import RenderCore.Utils.Constants;
import RenderCore.Types.DeviceProperties;
import RenderCore.Managers.CommandsManager;
import RenderCore.Managers.BufferManager;
import RenderCore.Managers.DeviceManager;
import RenderCore.Managers.PipelineManager;
import RenderCore.Managers.ShaderManager;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.DebugHelpers;
import RenderCore.Utils.EnumHelpers;

using namespace RenderCore;

EngineCore::EngineCore()
    : m_Instance(VK_NULL_HANDLE),
      m_Surface(VK_NULL_HANDLE),
      m_StateFlags(EngineCoreStateFlags::NONE),
      m_CameraMatrix(lookAt(glm::vec3(2.F, 2.F, 2.F), glm::vec3(0.F, 0.F, 0.F), glm::vec3(0.F, 0.F, 1.F))),
      m_ObjectID(0U)
#ifdef _DEBUG
      ,
      m_DebugMessenger(VK_NULL_HANDLE)
#endif
{
}

EngineCore::~EngineCore()
{
    try
    {
        Shutdown();
    }
    catch (...)
    {
    }
}

EngineCore& EngineCore::Get()
{
    static EngineCore Instance {};
    return Instance;
}

void EngineCore::Initialize(GLFWwindow* const Window)
{
    if (IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing vulkan render core";

    Helpers::CheckVulkanResult(volkInitialize());

#ifdef _DEBUG
    Helpers::ListAvailableInstanceLayers();

    for (char const* const& RequiredLayerIter: g_RequiredInstanceLayers)
    {
        Helpers::ListAvailableInstanceLayerExtensions(RequiredLayerIter);
    }

    for (char const* const& DebugLayerIter: g_DebugInstanceLayers)
    {
        Helpers::ListAvailableInstanceLayerExtensions(DebugLayerIter);
    }
#endif

    CreateVulkanInstance();
    CreateVulkanSurface(Window);
    InitializeRenderCore();
}

void EngineCore::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    Helpers::RemoveFlags(m_StateFlags, EngineCoreStateFlags::INITIALIZED);

    ShaderManager::Get().Shutdown();
    CommandsManager::Get().Shutdown();
    BufferManager::Get().Shutdown();
    PipelineManager::Get().Shutdown();
    DeviceManager::Get().Shutdown();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan render core";

#ifdef _DEBUG
    if (m_DebugMessenger != VK_NULL_HANDLE)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan debug messenger";

        DebugHelpers::DestroyDebugUtilsMessenger(m_Instance, m_DebugMessenger, nullptr);
        m_DebugMessenger = VK_NULL_HANDLE;
    }
#endif

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    m_Surface = VK_NULL_HANDLE;

    vkDestroyInstance(m_Instance, nullptr);
    m_Instance = VK_NULL_HANDLE;
}

void EngineCore::DrawFrame(GLFWwindow* const Window) const
{
    if (!IsInitialized())
    {
        return;
    }

    if (Window == nullptr)
    {
        throw std::runtime_error("Window is invalid");
    }

    constexpr EngineCoreStateFlags
            InvalidStatesToRender
            = EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE
              | EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION
              | EngineCoreStateFlags::PENDING_RESOURCES_CREATION
              | EngineCoreStateFlags::PENDING_PIPELINE_REFRESH;

    if (Helpers::HasAnyFlag(m_StateFlags, InvalidStatesToRender))
    {
        if (Helpers::HasFlag(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION))
        {
            CommandsManager::Get().DestroySynchronizationObjects();
            BufferManager::Get().DestroyResources(false);
            PipelineManager::Get().DestroyResources();

            Helpers::RemoveFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
            Helpers::AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION);
        }

        if (Helpers::HasFlag(m_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE))
        {
            if (DeviceManager::Get().UpdateDeviceProperties(Window))
            {
                BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Device properties updated, starting to draw frames with new properties";
                Helpers::RemoveFlags(m_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
            }
        }

        if (Helpers::HasFlag(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION))
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Refreshing resources...";
            BufferManager::Get().CreateSwapChain();
            BufferManager::Get().CreateDepthResources();
            BufferManager::Get().LoadImGuiFonts();
            CommandsManager::Get().CreateSynchronizationObjects();

            Helpers::RemoveFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION);
            Helpers::AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_PIPELINE_REFRESH);
        }

        if (Helpers::HasFlag(m_StateFlags, EngineCoreStateFlags::PENDING_PIPELINE_REFRESH))
        {
            PipelineManager::Get().CreateRenderPass();
            PipelineManager::Get().CreateDescriptorSetLayout();
            PipelineManager::Get().CreateGraphicsPipeline();

            BufferManager::Get().CreateFrameBuffers();

            PipelineManager::Get().CreateDescriptorPool();
            PipelineManager::Get().CreateDescriptorSets();

            Helpers::RemoveFlags(m_StateFlags, EngineCoreStateFlags::PENDING_PIPELINE_REFRESH);
        }
    }

    if (!Helpers::HasAnyFlag(m_StateFlags, InvalidStatesToRender))
    {
        if (std::optional<std::int32_t> const ImageIndice = TryRequestDrawImage();
            ImageIndice.has_value())
        {
            CommandsManager::Get().RecordCommandBuffers(ImageIndice.value());
            CommandsManager::Get().SubmitCommandBuffers();
            CommandsManager::Get().PresentFrame(ImageIndice.value());
        }
    }
}

bool EngineCore::IsInitialized() const
{
    return Helpers::HasFlag(m_StateFlags, EngineCoreStateFlags::INITIALIZED);
}

void EngineCore::LoadScene(std::string_view const ModelPath, std::string_view const TexturePath)
{
    if (!IsInitialized())
    {
        return;
    }

    if (!std::filesystem::exists(ModelPath))
    {
        throw std::runtime_error("Model path is invalid");
    }

    if (!std::filesystem::exists(TexturePath))
    {
        throw std::runtime_error("Texture path is invalid");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading scene...";

    m_ObjectID = BufferManager::Get().LoadObject(ModelPath, TexturePath);

    Helpers::AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
    Helpers::AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION);
}

void EngineCore::UnloadScene() const
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Unloading scene...";

    BufferManager::Get().UnLoadObject(m_ObjectID);

    Helpers::AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
}

VkInstance& EngineCore::GetInstance()
{
    return m_Instance;
}

VkSurfaceKHR& EngineCore::GetSurface()
{
    return m_Surface;
}

glm::mat4 EngineCore::GetCameraMatrix() const
{
    return m_CameraMatrix;
}

void EngineCore::SetCameraMatrix(glm::mat4 const& Value)
{
    m_CameraMatrix = Value;
}

std::optional<std::int32_t> EngineCore::TryRequestDrawImage() const
{
    if (!DeviceManager::Get().GetDeviceProperties().IsValid())
    {
        Helpers::AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
        Helpers::AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);

        return std::nullopt;
    }
    Helpers::RemoveFlags(m_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);

    std::optional<std::int32_t> const Output = CommandsManager::Get().DrawFrame();

    if (!Output.has_value())
    {
        Helpers::AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }
    else
    {
        Helpers::RemoveFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }

    return Output;
}

void EngineCore::CreateVulkanInstance()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan instance";

    constexpr VkApplicationInfo AppInfo {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName   = "VulkanApp",
            .applicationVersion = VK_MAKE_VERSION(1U, 0U, 0U),
            .pEngineName        = "No Engine",
            .engineVersion      = VK_MAKE_VERSION(1U, 0U, 0U),
            .apiVersion         = VK_API_VERSION_1_0};

    VkInstanceCreateInfo CreateInfo {
            .sType             = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext             = nullptr,
            .pApplicationInfo  = &AppInfo,
            .enabledLayerCount = 0U};

    std::vector Layers(g_RequiredInstanceLayers.begin(), g_RequiredInstanceLayers.end());
    std::vector<char const*> Extensions = Helpers::GetGLFWExtensions();
    Extensions.insert(Extensions.end(), g_RequiredInstanceExtensions.begin(), g_RequiredInstanceExtensions.end());

#ifdef _DEBUG
    VkValidationFeaturesEXT const ValidationFeatures = DebugHelpers::GetInstanceValidationFeatures();
    CreateInfo.pNext                                 = &ValidationFeatures;

    Layers.insert(Layers.end(), g_DebugInstanceLayers.begin(), g_DebugInstanceLayers.end());
    Extensions.insert(Extensions.end(), g_DebugInstanceExtensions.begin(), g_DebugInstanceExtensions.end());

    VkDebugUtilsMessengerCreateInfoEXT CreateDebugInfo {};
    DebugHelpers::PopulateDebugInfo(CreateDebugInfo);
#endif

    CreateInfo.enabledLayerCount   = static_cast<std::uint32_t>(Layers.size());
    CreateInfo.ppEnabledLayerNames = Layers.data();

    CreateInfo.enabledExtensionCount   = static_cast<std::uint32_t>(Extensions.size());
    CreateInfo.ppEnabledExtensionNames = Extensions.data();

    Helpers::CheckVulkanResult(vkCreateInstance(&CreateInfo, nullptr, &m_Instance));
    volkLoadInstance(m_Instance);

#ifdef _DEBUG
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Setting up debug messages";
    Helpers::CheckVulkanResult(DebugHelpers::CreateDebugUtilsMessenger(m_Instance, &CreateDebugInfo, nullptr, &m_DebugMessenger));
#endif
}

void EngineCore::CreateVulkanSurface(GLFWwindow* const Window)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan surface";

    if (m_Instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan instance is invalid.");
    }

    Helpers::CheckVulkanResult(glfwCreateWindowSurface(m_Instance, Window, nullptr, &m_Surface));
}

void EngineCore::InitializeRenderCore() const
{
    DeviceManager::Get().PickPhysicalDevice();
    DeviceManager::Get().CreateLogicalDevice();
    volkLoadDevice(DeviceManager::Get().GetLogicalDevice());

    BufferManager::Get().CreateMemoryAllocator();
    CompileDefaultShaders();

    Helpers::AddFlags(m_StateFlags, EngineCoreStateFlags::INITIALIZED);
    Helpers::AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION);
}

std::vector<VkPipelineShaderStageCreateInfo> EngineCore::CompileDefaultShaders()
{
    constexpr std::array FragmentShaders {
            DEBUG_SHADER_FRAG};
    constexpr std::array VertexShaders {
            DEBUG_SHADER_VERT};

    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

    VkDevice const& VulkanLogicalDevice = DeviceManager::Get().GetLogicalDevice();

    for (char const* const& FragmentShaderIter: FragmentShaders)
    {
        if (std::vector<std::uint32_t> FragmentShaderCode;
            ShaderManager::CompileOrLoadIfExists(FragmentShaderIter, FragmentShaderCode))
        {
            VkShaderModule const FragmentModule = ShaderManager::Get().CreateModule(VulkanLogicalDevice, FragmentShaderCode, EShLangFragment);
            ShaderStages.push_back(ShaderManager::Get().GetStageInfo(FragmentModule));
        }
    }

    for (char const* const& VertexShaderIter: VertexShaders)
    {
        if (std::vector<std::uint32_t> VertexShaderCode;
            ShaderManager::CompileOrLoadIfExists(VertexShaderIter, VertexShaderCode))
        {
            VkShaderModule const VertexModule = ShaderManager::Get().CreateModule(VulkanLogicalDevice, VertexShaderCode, EShLangVertex);
            ShaderStages.push_back(ShaderManager::Get().GetStageInfo(VertexModule));
        }
    }

    return ShaderStages;
}