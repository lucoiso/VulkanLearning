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
#include <boost/log/trivial.hpp>
#include <boost/bind/bind.hpp>
#include <set>
#include <thread>
#include <filesystem>

using namespace RenderCore;

VulkanRender::VulkanRender()
    : m_DeviceManager(std::make_unique<VulkanDeviceManager>()),
    m_PipelineManager(std::make_unique<VulkanPipelineManager>()),
    m_BufferManager(std::make_unique<VulkanBufferManager>()),
    m_CommandsManager(std::make_unique<VulkanCommandsManager>()),
    m_ShaderManager(std::make_unique<VulkanShaderManager>()),
    m_Instance(VK_NULL_HANDLE),
    m_Surface(VK_NULL_HANDLE),
    bIsSceneDirty(true),
    bIsSwapChainInvalidated(true),
    bHasLoadedScene(false)
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

bool VulkanRender::Initialize(GLFWwindow *const Window)
{
    if (IsInitialized())
    {
        return false;
    }

    if (!Window)
    {
        throw std::runtime_error("GLFW Window is invalid");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing vulkan render";

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

    return InitializeRenderCore(Window);
}

void VulkanRender::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

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

void VulkanRender::DrawFrame(GLFWwindow *const Window)
{
    if (!IsInitialized())
    {
        return;
    }

    if (Window == nullptr)
    {
        throw std::runtime_error("GLFW Window is invalid");
    }

    if (!VulkanRenderSubsystem::Get()->SetDeviceProperties(m_DeviceManager->GetPreferredProperties(Window)))
    {
        // TODO: Add bit flag: Invalid Device Properties
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }
    else
    {
        // TODO: Remove bit flag: Invalid Device Properties
    }

    // TODO: Check bit flags: Scene Dirty, Swap Chain Invalidated and Invalid Device Properties

    const std::int32_t ImageIndice = m_CommandsManager->DrawFrame(m_BufferManager->GetSwapChain());

    if (!VulkanRenderSubsystem::Get()->GetDeviceProperties().IsValid() || ImageIndice < 0)
    {
        if (!bIsSwapChainInvalidated)
        {
            m_CommandsManager->DestroySynchronizationObjects();
            m_BufferManager->DestroyResources(false);
            bIsSwapChainInvalidated = true;
        }
        else
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Refreshing device properties & capabilities...";

            m_BufferManager->CreateSwapChain();
            bIsSwapChainInvalidated = false;
            m_BufferManager->CreateDepthResources();
            m_BufferManager->CreateFrameBuffers(m_PipelineManager->GetRenderPass());
            m_CommandsManager->CreateSynchronizationObjects();

            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Buffers updated, starting to draw frames with new surface properties";
        }
    }
    else
    {
        m_BufferManager->UpdateUniformBuffers();

        const VulkanBufferRecordParameters Parameters = GetBufferRecordParameters(ImageIndice, 0u);
        m_CommandsManager->RecordCommandBuffers(Parameters);

        const VkQueue &GraphicsQueue = VulkanRenderSubsystem::Get()->GetQueueFromType(VulkanQueueType::Graphics);
        m_CommandsManager->SubmitCommandBuffers(GraphicsQueue);
        m_CommandsManager->PresentFrame(GraphicsQueue, m_BufferManager->GetSwapChain(), ImageIndice);
    }
}

bool VulkanRender::IsInitialized() const
{
    return m_DeviceManager && m_DeviceManager->IsInitialized()
        && m_BufferManager && m_BufferManager->IsInitialized()
        && m_ShaderManager
        && m_Instance != VK_NULL_HANDLE
        && m_Surface != VK_NULL_HANDLE;
}

void VulkanRender::LoadScene(const std::string_view ModelPath, const std::string_view TexturePath)
{
    if (!IsInitialized() || bHasLoadedScene)
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

    const std::uint64_t ObjectID = m_BufferManager->LoadObject(ModelPath);
    m_BufferManager->LoadTexture(TexturePath, ObjectID);

    m_BufferManager->CreateSwapChain();
    bIsSwapChainInvalidated = false;
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

    bIsSceneDirty = false;
    bHasLoadedScene = true;
}

void VulkanRender::UnloadScene()
{
    if (!IsInitialized() || !bHasLoadedScene)
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Unloading scene...";

    m_CommandsManager->DestroySynchronizationObjects();
    m_BufferManager->DestroyResources(true);
    m_PipelineManager->DestroyResources();

    bHasLoadedScene = false;
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

    std::vector<const char *> Layers(g_RequiredInstanceLayers.begin(), g_RequiredInstanceLayers.end());
    std::vector<const char *> Extensions = GetGLFWExtensions();

    for (const char *const &ExtensionIter : g_RequiredInstanceExtensions)
    {
        Extensions.push_back(ExtensionIter);
    }

#ifdef _DEBUG
    const VkValidationFeaturesEXT ValidationFeatures = GetInstanceValidationFeatures();
    CreateInfo.pNext = &ValidationFeatures;

    for (const char *const &DebugInstanceLayerIter : g_DebugInstanceLayers)
    {
        Layers.push_back(DebugInstanceLayerIter);
    }

    VkDebugUtilsMessengerCreateInfoEXT CreateDebugInfo{};
    PopulateDebugInfo(CreateDebugInfo);

    for (const char *const &DebugInstanceExtensionIter : g_DebugInstanceExtensions)
    {
        Extensions.push_back(DebugInstanceExtensionIter);
    }
#endif

    CreateInfo.enabledLayerCount = static_cast<std::uint32_t>(Layers.size());
    CreateInfo.ppEnabledLayerNames = Layers.data();

    CreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(Extensions.size());
    CreateInfo.ppEnabledExtensionNames = Extensions.data();

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateInstance(&CreateInfo, nullptr, &m_Instance));
    VulkanRenderSubsystem::Get()->SetInstance(m_Instance);

#ifdef _DEBUG
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Setting up debug messages";
    RENDERCORE_CHECK_VULKAN_RESULT(CreateDebugUtilsMessenger(m_Instance, &CreateDebugInfo, nullptr, &m_DebugMessenger));
#endif
}

void VulkanRender::CreateVulkanSurface(GLFWwindow *const Window)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan surface";

    if (!Window)
    {
        throw std::runtime_error("GLFW Window is invalid.");
    }

    if (m_Instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan instance is invalid.");
    }

    RENDERCORE_CHECK_VULKAN_RESULT(glfwCreateWindowSurface(m_Instance, Window, nullptr, &m_Surface));
    VulkanRenderSubsystem::Get()->SetSurface(m_Surface);
}

bool VulkanRender::InitializeRenderCore(GLFWwindow *const Window)
{
    if (!Window)
    {
        throw std::runtime_error("GLFW Window is invalid.");
    }

    m_DeviceManager->PickPhysicalDevice();
    m_DeviceManager->CreateLogicalDevice();
    VulkanRenderSubsystem::Get()->SetDeviceProperties(m_DeviceManager->GetPreferredProperties(Window));

    m_BufferManager->CreateMemoryAllocator();
    VulkanRenderSubsystem::Get()->SetDefaultShadersStageInfos(CompileDefaultShaders());

    return IsInitialized();
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
