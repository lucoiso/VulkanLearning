// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "VulkanRender.h"
#include "Managers/VulkanRenderSubsystem.h"
#include "Managers/VulkanDeviceManager.h"
#include "Managers/VulkanPipelineManager.h"
#include "Managers/VulkanBufferManager.h"
#include "Managers/VulkanCommandsManager.h"
#include "Managers/VulkanShaderManager.h"
#include "Utils/VulkanDebugHelpers.h"
#include "Utils/VulkanConstants.h"
#include "Utils/RenderCoreHelpers.h"
#include <set>
#include <thread>
#include <filesystem>
#include <boost/log/trivial.hpp>
#include <algorithm>

#ifndef VOLK_IMPLEMENTATION
#define VOLK_IMPLEMENTATION
#endif
#include <volk.h>

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#endif

using namespace RenderCore;

VulkanRender::VulkanRender()
    : m_DeviceManager(std::make_unique<VulkanDeviceManager>())
    , m_PipelineManager(std::make_unique<VulkanPipelineManager>())
    , m_BufferManager(std::make_unique<VulkanBufferManager>())
    , m_CommandsManager(std::make_unique<VulkanCommandsManager>())
    , m_ShaderManager(std::make_unique<VulkanShaderManager>())
    , m_Instance(VK_NULL_HANDLE)
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

void VulkanRender::Initialize(const QWindow *const Window)
{
    if (IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing vulkan render";

    RENDERCORE_CHECK_VULKAN_RESULT(volkInitialize());
    CreateVulkanInstance();

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

    RENDERCORE_CHECK_VULKAN_RESULT(vkDeviceWaitIdle(VulkanRenderSubsystem::Get()->GetDevice()));

    m_ShaderManager.reset();
    m_CommandsManager.reset();
    m_BufferManager.reset();
    m_PipelineManager.reset();
    m_DeviceManager.reset();

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

void VulkanRender::DrawFrame(const QWindow *const Window)
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
            m_CommandsManager->DestroySynchronizationObjects();
            m_BufferManager->DestroyResources(false);

            AddFlags(StateFlags, VulkanRenderStateFlags::PENDING_REFRESH);
        }
        else if (HasFlag(StateFlags, VulkanRenderStateFlags::PENDING_REFRESH) && !HasInvalidFlags)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Refreshing resources...";
            m_BufferManager->CreateSwapChain(true);
            m_BufferManager->CreateDepthResources();
            m_BufferManager->CreateFrameBuffers(m_PipelineManager->GetRenderPass());
            m_CommandsManager->CreateSynchronizationObjects();
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Buffers updated, starting to draw frames with new surface properties";

            RemoveFlags(StateFlags, VulkanRenderStateFlags::PENDING_REFRESH);
        }
    }
    else if (HasFlag(StateFlags, VulkanRenderStateFlags::SCENE_LOADED))
    {
        const std::int32_t IndiceToProcess = ImageIndice.value();
        m_BufferManager->UpdateUniformBuffers();

        const VulkanBufferRecordParameters Parameters = GetBufferRecordParameters(IndiceToProcess, 0u);
        m_CommandsManager->RecordCommandBuffers(Parameters);

        const VkQueue &GraphicsQueue = VulkanRenderSubsystem::Get()->GetQueueFromType(VulkanQueueType::Graphics);
        m_CommandsManager->SubmitCommandBuffers(GraphicsQueue);
        m_CommandsManager->PresentFrame(GraphicsQueue, m_BufferManager->GetSwapChain(), IndiceToProcess);

        AddFlags(StateFlags, VulkanRenderStateFlags::RENDERING);
        RemoveFlags(StateFlags, VulkanRenderStateFlags::PENDING_REFRESH);
    }
}

std::optional<std::int32_t> VulkanRender::TryRequestDrawImage(const QWindow *const Window) const
{
    if (!VulkanRenderSubsystem::Get()->SetDeviceProperties(m_DeviceManager->GetPreferredProperties(Window)))
    {
        RemoveFlags(StateFlags, VulkanRenderStateFlags::RENDERING);
        AddFlags(StateFlags, VulkanRenderStateFlags::INVALID_PROPERTIES);
        
        constexpr std::uint64_t DelayMs = 250u;
        std::this_thread::sleep_for(std::chrono::milliseconds(DelayMs));

        return std::optional<std::int32_t>();
    }
    else
    {
        RemoveFlags(StateFlags, VulkanRenderStateFlags::INVALID_PROPERTIES);
    }

    const std::optional<std::int32_t> Output = m_CommandsManager->DrawFrame(m_BufferManager->GetSwapChain());

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
    
    m_PipelineManager->DestroyResources();

    const std::uint64_t ObjectID = m_BufferManager->LoadObject(ModelPath);
    m_BufferManager->LoadTexture(TexturePath, ObjectID);

    m_BufferManager->CreateSwapChain(false);
    m_BufferManager->CreateDepthResources();

    m_PipelineManager->CreateRenderPass();
    m_PipelineManager->CreateDescriptorSetLayout();
    m_PipelineManager->CreateGraphicsPipeline(VulkanRenderSubsystem::Get()->GetDefaultShadersStageInfos());

    m_BufferManager->CreateFrameBuffers(m_PipelineManager->GetRenderPass());

    std::vector<VulkanTextureData> TextureData;
    m_BufferManager->GetObjectTexture(ObjectID, TextureData);

    const VulkanObjectData Data{
        .UniformBuffers = m_BufferManager->GetUniformBuffers(ObjectID),
        .TextureDatas = TextureData};

    m_PipelineManager->CreateDescriptorPool();
    m_PipelineManager->CreateDescriptorSets({Data});

    m_CommandsManager->CreateSynchronizationObjects();

    AddFlags(StateFlags, VulkanRenderStateFlags::SCENE_LOADED);
}

void VulkanRender::UnloadScene()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Unloading scene...";

    m_CommandsManager->DestroySynchronizationObjects();
    m_BufferManager->DestroyResources(true);
    m_PipelineManager->DestroyResources();

    RemoveFlags(StateFlags, VulkanRenderStateFlags::SCENE_LOADED);
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
        
    const std::vector<std::string> AvailableInstanceLayers = GetAvailableInstanceLayersNames();
    std::vector<const char*> Layers(g_RequiredInstanceLayers.begin(), g_RequiredInstanceLayers.end());
    std::erase_if(Layers, 
        [&AvailableInstanceLayers](const std::string &LayerIter) 
        {
            return std::find(AvailableInstanceLayers.begin(), AvailableInstanceLayers.end(), LayerIter) == AvailableInstanceLayers.end();
        }
    );

    const std::vector<std::string> AvailableInstanceExtensions = GetAvailableInstanceExtensionsNames();
    std::vector<const char*> Extensions(g_RequiredInstanceExtensions.begin(), g_RequiredInstanceExtensions.end());
    std::erase_if(Extensions, 
        [&AvailableInstanceExtensions](const std::string &ExtensionIter) 
        {
            return std::find(AvailableInstanceExtensions.begin(), AvailableInstanceExtensions.end(), ExtensionIter) == AvailableInstanceExtensions.end();
        }
    );

#ifdef _DEBUG
    bool bSupportsDebuggingFeatures = false;

    const VkValidationFeaturesEXT ValidationFeatures = GetInstanceValidationFeatures();
    CreateInfo.pNext = &ValidationFeatures;

    for (const char *const &DebugInstanceLayerIter : g_DebugInstanceLayers)
    {
        if (std::find(AvailableInstanceLayers.begin(), AvailableInstanceLayers.end(), DebugInstanceLayerIter) != AvailableInstanceLayers.end())
        {
            Layers.push_back(DebugInstanceLayerIter);

            if (!bSupportsDebuggingFeatures)
            {
                bSupportsDebuggingFeatures = true;
            }
        }
    }

    VkDebugUtilsMessengerCreateInfoEXT CreateDebugInfo{};
    PopulateDebugInfo(CreateDebugInfo);

    for (const char *const &DebugInstanceExtensionIter : g_DebugInstanceExtensions)
    {
        if (std::find(AvailableInstanceExtensions.begin(), AvailableInstanceExtensions.end(), DebugInstanceExtensionIter) != AvailableInstanceExtensions.end())
        {
            Extensions.push_back(DebugInstanceExtensionIter);

            if (!bSupportsDebuggingFeatures)
            {
                bSupportsDebuggingFeatures = true;
            }
        }
    }
#endif

    CreateInfo.enabledLayerCount = static_cast<std::uint32_t>(Layers.size());
    CreateInfo.ppEnabledLayerNames = Layers.data();

    CreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(Extensions.size());
    CreateInfo.ppEnabledExtensionNames = Extensions.data();

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateInstance(&CreateInfo, nullptr, &m_Instance));
    VulkanRenderSubsystem::Get()->SetInstance(m_Instance);
    volkLoadInstance(m_Instance);

#ifdef _DEBUG
    if (bSupportsDebuggingFeatures)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Setting up debug messages";
        RENDERCORE_CHECK_VULKAN_RESULT(CreateDebugUtilsMessenger(m_Instance, &CreateDebugInfo, nullptr, &m_DebugMessenger));
    }
#endif
}

void VulkanRender::CreateVulkanSurface(const QWindow *const Window)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan surface";

    if (m_Instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan instance is invalid.");
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    const VkWin32SurfaceCreateInfoKHR CreateInfo{
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0u,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = reinterpret_cast<HWND>(Window->winId())};
    
    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateWin32SurfaceKHR(m_Instance, &CreateInfo, nullptr, &m_Surface));
#endif

    VulkanRenderSubsystem::Get()->SetSurface(m_Surface);
}

void VulkanRender::InitializeRenderCore(const QWindow *const Window)
{
    m_DeviceManager->PickPhysicalDevice();
    m_DeviceManager->CreateLogicalDevice();
    volkLoadDevice(VulkanRenderSubsystem::Get()->GetDevice());

    VulkanRenderSubsystem::Get()->SetDeviceProperties(m_DeviceManager->GetPreferredProperties(Window));

    m_BufferManager->CreateMemoryAllocator();
    VulkanRenderSubsystem::Get()->SetDefaultShadersStageInfos(CompileDefaultShaders());
    
    AddFlags(StateFlags, VulkanRenderStateFlags::INITIALIZED);
}

std::vector<VkPipelineShaderStageCreateInfo> VulkanRender::CompileDefaultShaders()
{
    constexpr std::array<const char *, 1u> FragmentShaders { DEBUG_SHADER_FRAG };
    constexpr std::array<const char *, 1u> VertexShaders { DEBUG_SHADER_VERT };
    
    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

    const VkDevice &VulkanLogicalDevice = VulkanRenderSubsystem::Get()->GetDevice();

    for (const char *const &FragmentShaderIter : FragmentShaders)
    {
        if (std::vector<std::uint32_t> FragmentShaderCode; m_ShaderManager->CompileOrLoadIfExists(FragmentShaderIter, FragmentShaderCode))
        {
            const VkShaderModule FragmentModule = m_ShaderManager->CreateModule(VulkanLogicalDevice, FragmentShaderCode, EShLangFragment);
            ShaderStages.push_back(m_ShaderManager->GetStageInfo(FragmentModule));
        }
    }

    for (const char *const &VertexShaderIter : VertexShaders)
    {
        if (std::vector<std::uint32_t> VertexShaderCode; m_ShaderManager->CompileOrLoadIfExists(VertexShaderIter, VertexShaderCode))
        {
            const VkShaderModule VertexModule = m_ShaderManager->CreateModule(VulkanLogicalDevice, VertexShaderCode, EShLangVertex);
            ShaderStages.push_back(m_ShaderManager->GetStageInfo(VertexModule));
        }
    }

    return ShaderStages;
}

VulkanBufferRecordParameters VulkanRender::GetBufferRecordParameters(const std::uint32_t ImageIndex, const std::uint64_t ObjectID) const
{
    return {
        .RenderPass = m_PipelineManager->GetRenderPass(),
        .Pipeline = m_PipelineManager->GetPipeline(),
        .Extent = VulkanRenderSubsystem::Get()->GetDeviceProperties().Extent,
        .FrameBuffers = m_BufferManager->GetFrameBuffers(),
        .VertexBuffers = m_BufferManager->GetVertexBuffers(ObjectID),
        .IndexBuffers = m_BufferManager->GetIndexBuffers(ObjectID),
        .PipelineLayout = m_PipelineManager->GetPipelineLayout(),
        .DescriptorSets = m_PipelineManager->GetDescriptorSets(),
        .IndexCount = m_BufferManager->GetIndicesCount(ObjectID),
        .ImageIndex = ImageIndex,
        .Offsets = {0u}};
}
