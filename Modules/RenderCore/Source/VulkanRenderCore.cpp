// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "VulkanRenderCore.h"
#include "Managers/VulkanDeviceManager.h"
#include "Managers/VulkanPipelineManager.h"
#include "Managers/VulkanBufferManager.h"
#include "Managers/VulkanCommandsManager.h"
#include "Managers/VulkanShaderManager.h"
#include "Utils/VulkanDebugHelpers.h"
#include "Utils/VulkanConstants.h"
#include "Utils/RenderCoreHelpers.h"
#include <filesystem>
#include <boost/log/trivial.hpp>

#ifndef VOLK_IMPLEMENTATION
#define VOLK_IMPLEMENTATION
#endif
#include <volk.h>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#endif

using namespace RenderCore;

VulkanRenderCore VulkanRenderCore::Instance;

VulkanRenderCore::VulkanRenderCore()
    : m_Instance(VK_NULL_HANDLE)
    , m_Surface(VK_NULL_HANDLE)
    , StateFlags(VulkanRenderCoreStateFlags::NONE)
#ifdef _DEBUG
    , m_DebugMessenger(VK_NULL_HANDLE)
#endif
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan render core";
}

VulkanRenderCore::~VulkanRenderCore()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan render core";
    Shutdown();
}

VulkanRenderCore &VulkanRenderCore::Get()
{
    return Instance;
}

void VulkanRenderCore::Initialize(GLFWwindow *const Window)
{
    if (IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing vulkan render core";

    RENDERCORE_CHECK_VULKAN_RESULT(volkInitialize());

#ifdef _DEBUG
    RenderCoreHelpers::ListAvailableInstanceLayers();

    for (const char *const &RequiredLayerIter : g_RequiredInstanceLayers)
    {
        RenderCoreHelpers::ListAvailableInstanceLayerExtensions(RequiredLayerIter);
    }

    for (const char *const &DebugLayerIter : g_DebugInstanceLayers)
    {
        RenderCoreHelpers::ListAvailableInstanceLayerExtensions(DebugLayerIter);
    }
#endif

    CreateVulkanInstance();
    CreateVulkanSurface(Window);
    InitializeRenderCore(Window);
}

void VulkanRenderCore::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::INITIALIZED);
    RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::SHUTDOWN);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan render core";

    RENDERCORE_CHECK_VULKAN_RESULT(vkDeviceWaitIdle(VulkanDeviceManager::Get().GetLogicalDevice()));

    VulkanShaderManager::Get().Shutdown();
    VulkanCommandsManager::Get().Shutdown();
    VulkanBufferManager::Get().Shutdown();
    VulkanPipelineManager::Get().Shutdown();
    VulkanDeviceManager::Get().Shutdown();

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

void VulkanRenderCore::DrawFrame(GLFWwindow *const Window)
{
    if (!IsInitialized())
    {
        return;
    }

    if (Window == nullptr)
    {
        throw std::runtime_error("Window is invalid");
    }

    const std::optional<std::int32_t> ImageIndice = TryRequestDrawImage(Window);
    const bool HasInvalidFlags = RenderCoreHelpers::HasFlag(StateFlags, VulkanRenderCoreStateFlags::INVALID_PROPERTIES) || RenderCoreHelpers::HasFlag(StateFlags, VulkanRenderCoreStateFlags::INVALID_RESOURCES);

    if (!ImageIndice.has_value() || RenderCoreHelpers::HasFlag(StateFlags, VulkanRenderCoreStateFlags::PENDING_REFRESH) || HasInvalidFlags)
    {
        RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::RENDERING);

        if (!RenderCoreHelpers::HasFlag(StateFlags, VulkanRenderCoreStateFlags::PENDING_REFRESH) && HasInvalidFlags)
        {
            VulkanCommandsManager::Get().DestroySynchronizationObjects();
            VulkanBufferManager::Get().DestroyResources(false);

            RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_REFRESH);
        }
        else if (RenderCoreHelpers::HasFlag(StateFlags, VulkanRenderCoreStateFlags::PENDING_REFRESH) && !HasInvalidFlags)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Refreshing resources...";
            VulkanBufferManager::Get().CreateSwapChain();
            VulkanBufferManager::Get().CreateDepthResources();
            VulkanBufferManager::Get().CreateFrameBuffers();
            VulkanCommandsManager::Get().CreateSynchronizationObjects();
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Buffers updated, starting to draw frames with new surface properties";

            RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_REFRESH);
        }
    }
    else if (RenderCoreHelpers::HasFlag(StateFlags, VulkanRenderCoreStateFlags::SCENE_LOADED))
    {
        VulkanCommandsManager::Get().RecordCommandBuffers(ImageIndice.value());
        VulkanCommandsManager::Get().SubmitCommandBuffers();
        VulkanCommandsManager::Get().PresentFrame(ImageIndice.value());

        RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::RENDERING);
        RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_REFRESH);
    }
}

bool VulkanRenderCore::IsInitialized() const
{
    return RenderCoreHelpers::HasFlag(StateFlags, VulkanRenderCoreStateFlags::INITIALIZED);
}

void VulkanRenderCore::LoadScene(const std::string_view ModelPath, const std::string_view TexturePath)
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
    
    VulkanPipelineManager::Get().DestroyResources();

    VulkanBufferManager::Get().CreateSwapChain();

    const std::uint64_t ObjectID = VulkanBufferManager::Get().LoadObject(ModelPath, TexturePath);
    VulkanBufferManager::Get().CreateDepthResources();

    VulkanPipelineManager::Get().CreateRenderPass();
    VulkanPipelineManager::Get().CreateDescriptorSetLayout();
    VulkanPipelineManager::Get().CreateGraphicsPipeline();

    VulkanBufferManager::Get().CreateFrameBuffers();

    VulkanPipelineManager::Get().CreateDescriptorPool();
    VulkanPipelineManager::Get().CreateDescriptorSets();

    VulkanCommandsManager::Get().CreateSynchronizationObjects();

    RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::SCENE_LOADED);
}

void VulkanRenderCore::UnloadScene()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Unloading scene...";

    VulkanCommandsManager::Get().DestroySynchronizationObjects();
    VulkanBufferManager::Get().DestroyResources(true);
    VulkanPipelineManager::Get().DestroyResources();

    RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::SCENE_LOADED);
}

VkInstance &VulkanRenderCore::GetInstance()
{
    return m_Instance;
}

VkSurfaceKHR &VulkanRenderCore::GetSurface()
{
    return m_Surface;
}

VulkanRenderCoreStateFlags VulkanRenderCore::GetStateFlags() const
{
    return StateFlags;
}

std::optional<std::int32_t> VulkanRenderCore::TryRequestDrawImage(GLFWwindow *const Window)
{
    if (!VulkanDeviceManager::Get().UpdateDeviceProperties(Window))
    {
        RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::RENDERING);
        RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::INVALID_PROPERTIES);

        return std::optional<std::int32_t>();
    }
    else
    {
        RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::INVALID_PROPERTIES);
    }

    const std::optional<std::int32_t> Output = VulkanCommandsManager::Get().DrawFrame();

    if (!Output.has_value())
    {
        RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::RENDERING);
        RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::INVALID_RESOURCES);
    }
    else
    {
        RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::INVALID_RESOURCES);
    }

    return Output;
}

void VulkanRenderCore::CreateVulkanInstance()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan instance";

    const VkApplicationInfo AppInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "VulkanApp",
        .applicationVersion = VK_MAKE_VERSION(1u, 0u, 0u),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1u, 0u, 0u),
        .apiVersion = VK_API_VERSION_1_0};

    VkInstanceCreateInfo CreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &AppInfo,
        .enabledLayerCount = 0u};
        
    std::vector<const char*> Layers(g_RequiredInstanceLayers.begin(), g_RequiredInstanceLayers.end());
    std::vector<const char*> Extensions = RenderCoreHelpers::GetGLFWExtensions();
    Extensions.insert(Extensions.end(), g_RequiredInstanceExtensions.begin(), g_RequiredInstanceExtensions.end());

#ifdef _DEBUG
    const VkValidationFeaturesEXT ValidationFeatures = DebugHelpers::GetInstanceValidationFeatures();
    CreateInfo.pNext = &ValidationFeatures;

    Layers.insert(Layers.end(), g_DebugInstanceLayers.begin(), g_DebugInstanceLayers.end());
    Extensions.insert(Extensions.end(), g_DebugInstanceExtensions.begin(), g_DebugInstanceExtensions.end());

    VkDebugUtilsMessengerCreateInfoEXT CreateDebugInfo{};
    DebugHelpers::PopulateDebugInfo(CreateDebugInfo);
#endif

    CreateInfo.enabledLayerCount = static_cast<std::uint32_t>(Layers.size());
    CreateInfo.ppEnabledLayerNames = Layers.data();

    CreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(Extensions.size());
    CreateInfo.ppEnabledExtensionNames = Extensions.data();

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateInstance(&CreateInfo, nullptr, &m_Instance));
    volkLoadInstance(m_Instance);

#ifdef _DEBUG
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Setting up debug messages";
    RENDERCORE_CHECK_VULKAN_RESULT(DebugHelpers::CreateDebugUtilsMessenger(m_Instance, &CreateDebugInfo, nullptr, &m_DebugMessenger));
#endif
}

void VulkanRenderCore::CreateVulkanSurface(GLFWwindow *const Window)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan surface";

    if (m_Instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan instance is invalid.");
    }

    RENDERCORE_CHECK_VULKAN_RESULT(glfwCreateWindowSurface(m_Instance, Window, nullptr, &m_Surface));
}

void VulkanRenderCore::InitializeRenderCore(GLFWwindow *const Window)
{
    VulkanDeviceManager::Get().PickPhysicalDevice();
    VulkanDeviceManager::Get().CreateLogicalDevice();
    volkLoadDevice(VulkanDeviceManager::Get().GetLogicalDevice());

    VulkanBufferManager::Get().CreateMemoryAllocator();
    CompileDefaultShaders();

    RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::INITIALIZED);
}

std::vector<VkPipelineShaderStageCreateInfo> VulkanRenderCore::CompileDefaultShaders()
{
    constexpr std::array<const char *, 1u> FragmentShaders { DEBUG_SHADER_FRAG };
    constexpr std::array<const char *, 1u> VertexShaders { DEBUG_SHADER_VERT };
    
    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

    const VkDevice &VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    for (const char *const &FragmentShaderIter : FragmentShaders)
    {
        if (std::vector<std::uint32_t> FragmentShaderCode; VulkanShaderManager::Get().CompileOrLoadIfExists(FragmentShaderIter, FragmentShaderCode))
        {
            const VkShaderModule FragmentModule = VulkanShaderManager::Get().CreateModule(VulkanLogicalDevice, FragmentShaderCode, EShLangFragment);
            ShaderStages.push_back(VulkanShaderManager::Get().GetStageInfo(FragmentModule));
        }
    }

    for (const char *const &VertexShaderIter : VertexShaders)
    {
        if (std::vector<std::uint32_t> VertexShaderCode; VulkanShaderManager::Get().CompileOrLoadIfExists(VertexShaderIter, VertexShaderCode))
        {
            const VkShaderModule VertexModule = VulkanShaderManager::Get().CreateModule(VulkanLogicalDevice, VertexShaderCode, EShLangVertex);
            ShaderStages.push_back(VulkanShaderManager::Get().GetStageInfo(VertexModule));
        }
    }

    return ShaderStages;
}
