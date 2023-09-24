// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "VulkanRender.h"
#include "Managers/VulkanDeviceManager.h"
#include "Managers/VulkanPipelineManager.h"
#include "Managers/VulkanBufferManager.h"
#include "Managers/VulkanCommandsManager.h"
#include "Managers/VulkanShaderManager.h"
#include "Utils/VulkanDebugHelpers.h"
#include "Utils/VulkanConstants.h"
#include "Utils/RenderCoreHelpers.h"
#include <set>
#include <filesystem>
#include <boost/log/trivial.hpp>
#include <algorithm>
#include <GLFW/glfw3.h>

#ifndef VOLK_IMPLEMENTATION
#define VOLK_IMPLEMENTATION
#endif
#include <volk.h>

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#endif

using namespace RenderCore;

VulkanRender VulkanRender::Instance;

VulkanRender::VulkanRender()
    : m_Instance(VK_NULL_HANDLE)
    , m_Surface(VK_NULL_HANDLE)
    , StateFlags(VulkanRenderStateFlags::NONE)
#ifdef _DEBUG
    , m_DebugMessenger(VK_NULL_HANDLE)
#endif
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan render";
}

VulkanRender::~VulkanRender()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan render";
    Shutdown();
}

VulkanRender &VulkanRender::Get()
{
    return Instance;
}

void VulkanRender::Initialize(GLFWwindow *const Window)
{
    if (IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing vulkan render";

    RENDERCORE_CHECK_VULKAN_RESULT(volkInitialize());

#ifdef _DEBUG
    ListAvailableInstanceLayers();

    for (const char *const &RequiredLayerIter : g_RequiredInstanceLayers)
    {
        ListAvailableInstanceLayerExtensions(RequiredLayerIter);
    }

    for (const char *const &DebugLayerIter : g_DebugInstanceLayers)
    {
        ListAvailableInstanceLayerExtensions(DebugLayerIter);
    }
#endif

    CreateVulkanInstance();
    CreateVulkanSurface(Window);
    InitializeRenderCore(Window);
}

void VulkanRender::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    RemoveFlags(StateFlags, VulkanRenderStateFlags::INITIALIZED);
    AddFlags(StateFlags, VulkanRenderStateFlags::SHUTDOWN);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan render";

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

        DestroyDebugUtilsMessenger(m_Instance, m_DebugMessenger, nullptr);
        m_DebugMessenger = VK_NULL_HANDLE;
    }
#endif

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    m_Surface = VK_NULL_HANDLE;

    vkDestroyInstance(m_Instance, nullptr);
    m_Instance = VK_NULL_HANDLE;
}

void VulkanRender::DrawFrame(GLFWwindow *const Window)
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
    const bool HasInvalidFlags = HasFlag(StateFlags, VulkanRenderStateFlags::INVALID_PROPERTIES) || HasFlag(StateFlags, VulkanRenderStateFlags::INVALID_RESOURCES);

    if (!ImageIndice.has_value() || HasFlag(StateFlags, VulkanRenderStateFlags::PENDING_REFRESH) || HasInvalidFlags)
    {
        RemoveFlags(StateFlags, VulkanRenderStateFlags::RENDERING);

        if (!HasFlag(StateFlags, VulkanRenderStateFlags::PENDING_REFRESH) && HasInvalidFlags)
        {
            VulkanCommandsManager::Get().DestroySynchronizationObjects();
            VulkanBufferManager::Get().DestroyResources(false);

            AddFlags(StateFlags, VulkanRenderStateFlags::PENDING_REFRESH);
        }
        else if (HasFlag(StateFlags, VulkanRenderStateFlags::PENDING_REFRESH) && !HasInvalidFlags)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Refreshing resources...";
            VulkanBufferManager::Get().CreateSwapChain(true);
            VulkanBufferManager::Get().CreateDepthResources();
            VulkanBufferManager::Get().CreateFrameBuffers(VulkanPipelineManager::Get().GetRenderPass());
            VulkanCommandsManager::Get().CreateSynchronizationObjects();
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Buffers updated, starting to draw frames with new surface properties";

            RemoveFlags(StateFlags, VulkanRenderStateFlags::PENDING_REFRESH);
        }
    }
    else if (HasFlag(StateFlags, VulkanRenderStateFlags::SCENE_LOADED))
    {
        const std::int32_t IndiceToProcess = ImageIndice.value();
        const VulkanBufferRecordParameters Parameters = GetBufferRecordParameters(IndiceToProcess, 0u, Window);

        VulkanCommandsManager::Get().RecordCommandBuffers(Parameters);

        const VkQueue &GraphicsQueue = VulkanDeviceManager::Get().GetGraphicsQueue().second;
        VulkanCommandsManager::Get().SubmitCommandBuffers(GraphicsQueue);
        VulkanCommandsManager::Get().PresentFrame(GraphicsQueue, VulkanBufferManager::Get().GetSwapChain(), IndiceToProcess);

        AddFlags(StateFlags, VulkanRenderStateFlags::RENDERING);
        RemoveFlags(StateFlags, VulkanRenderStateFlags::PENDING_REFRESH);
    }
}

bool VulkanRender::IsInitialized() const
{
    return HasFlag(StateFlags, VulkanRenderStateFlags::INITIALIZED);
}

void VulkanRender::LoadScene(const std::string_view ModelPath, const std::string_view TexturePath)
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

    VulkanBufferManager::Get().CreateSwapChain(false);

    const std::uint64_t ObjectID = VulkanBufferManager::Get().LoadObject(ModelPath, TexturePath);
    VulkanBufferManager::Get().CreateDepthResources();

    VulkanPipelineManager::Get().CreateRenderPass();
    VulkanPipelineManager::Get().CreateDescriptorSetLayout();
    VulkanPipelineManager::Get().CreateGraphicsPipeline(VulkanShaderManager::Get().GetStageInfos());

    VulkanBufferManager::Get().CreateFrameBuffers(VulkanPipelineManager::Get().GetRenderPass());

    VulkanTextureData TextureData;
    VulkanBufferManager::Get().GetObjectTexture(ObjectID, TextureData);

    VulkanPipelineManager::Get().CreateDescriptorPool();
    VulkanPipelineManager::Get().CreateDescriptorSets({ TextureData });

    VulkanCommandsManager::Get().CreateSynchronizationObjects();

    AddFlags(StateFlags, VulkanRenderStateFlags::SCENE_LOADED);
}

void VulkanRender::UnloadScene()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Unloading scene...";

    VulkanCommandsManager::Get().DestroySynchronizationObjects();
    VulkanBufferManager::Get().DestroyResources(true);
    VulkanPipelineManager::Get().DestroyResources();

    RemoveFlags(StateFlags, VulkanRenderStateFlags::SCENE_LOADED);
}

VkInstance &VulkanRender::GetInstance()
{
    return m_Instance;
}

VkSurfaceKHR &VulkanRender::GetSurface()
{
    return m_Surface;
}

VulkanRenderStateFlags VulkanRender::GetStateFlags() const
{
    return StateFlags;
}

std::optional<std::int32_t> VulkanRender::TryRequestDrawImage(GLFWwindow *const Window)
{
    if (!VulkanDeviceManager::Get().UpdateDeviceProperties(Window))
    {
        RemoveFlags(StateFlags, VulkanRenderStateFlags::RENDERING);
        AddFlags(StateFlags, VulkanRenderStateFlags::INVALID_PROPERTIES);

        return std::optional<std::int32_t>();
    }
    else
    {
        RemoveFlags(StateFlags, VulkanRenderStateFlags::INVALID_PROPERTIES);
    }

    const std::optional<std::int32_t> Output = VulkanCommandsManager::Get().DrawFrame(VulkanBufferManager::Get().GetSwapChain());

    if (!Output.has_value())
    {
        RemoveFlags(StateFlags, VulkanRenderStateFlags::RENDERING);
        AddFlags(StateFlags, VulkanRenderStateFlags::INVALID_RESOURCES);
    }
    else
    {
        RemoveFlags(StateFlags, VulkanRenderStateFlags::INVALID_RESOURCES);
    }

    return Output;
}

void VulkanRender::CreateVulkanInstance()
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
    std::vector<const char*> Extensions = GetGLFWExtensions();
    Extensions.insert(Extensions.end(), g_RequiredInstanceExtensions.begin(), g_RequiredInstanceExtensions.end());

#ifdef _DEBUG
    const VkValidationFeaturesEXT ValidationFeatures = GetInstanceValidationFeatures();
    CreateInfo.pNext = &ValidationFeatures;

    Layers.insert(Layers.end(), g_DebugInstanceLayers.begin(), g_DebugInstanceLayers.end());
    Extensions.insert(Extensions.end(), g_DebugInstanceExtensions.begin(), g_DebugInstanceExtensions.end());

    VkDebugUtilsMessengerCreateInfoEXT CreateDebugInfo{};
    PopulateDebugInfo(CreateDebugInfo);
#endif

    CreateInfo.enabledLayerCount = static_cast<std::uint32_t>(Layers.size());
    CreateInfo.ppEnabledLayerNames = Layers.data();

    CreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(Extensions.size());
    CreateInfo.ppEnabledExtensionNames = Extensions.data();

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateInstance(&CreateInfo, nullptr, &m_Instance));
    volkLoadInstance(m_Instance);

#ifdef _DEBUG
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Setting up debug messages";
    RENDERCORE_CHECK_VULKAN_RESULT(CreateDebugUtilsMessenger(m_Instance, &CreateDebugInfo, nullptr, &m_DebugMessenger));
#endif
}

void VulkanRender::CreateVulkanSurface(GLFWwindow *const Window)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan surface";

    if (m_Instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan instance is invalid.");
    }

    RENDERCORE_CHECK_VULKAN_RESULT(glfwCreateWindowSurface(m_Instance, Window, nullptr, &m_Surface));
}

void VulkanRender::InitializeRenderCore(GLFWwindow *const Window)
{
    VulkanDeviceManager::Get().PickPhysicalDevice();
    VulkanDeviceManager::Get().CreateLogicalDevice();
    volkLoadDevice(VulkanDeviceManager::Get().GetLogicalDevice());

    VulkanBufferManager::Get().CreateMemoryAllocator();
    CompileDefaultShaders();

    AddFlags(StateFlags, VulkanRenderStateFlags::INITIALIZED);
}

std::vector<VkPipelineShaderStageCreateInfo> VulkanRender::CompileDefaultShaders()
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

VulkanBufferRecordParameters VulkanRender::GetBufferRecordParameters(const std::uint32_t ImageIndex, const std::uint64_t ObjectID, GLFWwindow *const Window) const
{
    return {
        .RenderPass = VulkanPipelineManager::Get().GetRenderPass(),
        .Pipeline = VulkanPipelineManager::Get().GetPipeline(),
        .Extent = VulkanDeviceManager::Get().GetDeviceProperties().Extent,
        .FrameBuffers = VulkanBufferManager::Get().GetFrameBuffers(),
        .VertexBuffer = VulkanBufferManager::Get().GetVertexBuffer(ObjectID),
        .IndexBuffer = VulkanBufferManager::Get().GetIndexBuffer(ObjectID),
        .PipelineLayout = VulkanPipelineManager::Get().GetPipelineLayout(),
        .DescriptorSets = VulkanPipelineManager::Get().GetDescriptorSets(),
        .IndexCount = VulkanBufferManager::Get().GetIndicesCount(ObjectID),
        .ImageIndex = ImageIndex,
        .Offsets = {0u}};
}
