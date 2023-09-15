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
#include <boost/log/trivial.hpp>
#include <boost/bind/bind.hpp>
#include <set>
#include <thread>
#include <filesystem>

using namespace RenderCore;

class VulkanRender::Impl
{
public:
    Impl(const Impl &) = delete;
    Impl &operator=(const Impl &) = delete;

    Impl()
        : m_DeviceManager(nullptr)
        , m_PipelineManager(nullptr)
        , m_BufferManager(nullptr)
        , m_CommandsManager(nullptr)
        , m_ShaderManager(nullptr)
        , m_Instance(VK_NULL_HANDLE)
        , m_Surface(VK_NULL_HANDLE)
        , m_SharedDeviceProperties()
        , m_DefaultShadersStageInfos({})
        , bIsSceneDirty(true)
        , bIsSwapChainInvalidated(true)
        , bHasLoadedScene(false)
#ifdef _DEBUG
        , m_DebugMessenger(VK_NULL_HANDLE)
#endif
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan render implementation";
    }

    ~Impl()
    {
        if (!IsInitialized())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan render implementation";
        Shutdown();
    }

    bool Initialize(GLFWwindow *const Window)
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

        for (const char* const &RequiredLayerIter : g_RequiredInstanceLayers)
        {
            ListAvailableInstanceLayerExtensions(RequiredLayerIter);
        }

        for (const char* const &DebugLayerIter : g_DebugInstanceLayers)
        {
            ListAvailableInstanceLayerExtensions(DebugLayerIter);
        }
#endif

        CreateVulkanInstance();
        CreateVulkanSurface(Window);

        return InitializeRenderCore(Window);
    }

    void Shutdown()
    {
        if (!IsInitialized())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan render";

        RENDERCORE_CHECK_VULKAN_RESULT(vkDeviceWaitIdle(m_DeviceManager->GetLogicalDevice()));

        m_ShaderManager->Shutdown();
        m_CommandsManager->Shutdown({m_DeviceManager->GetGraphicsQueue(), m_DeviceManager->GetPresentationQueue(), m_DeviceManager->GetTransferQueue()});
        m_BufferManager->Shutdown();
        m_PipelineManager->Shutdown();
        m_DeviceManager->Shutdown();

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

    void DrawFrame(GLFWwindow *const Window)
    {
        if (!IsInitialized())
        {
            return;
        }

        if (Window == nullptr)
        {
            throw std::runtime_error("GLFW Window is invalid");
        }

        const std::int32_t ImageIndice = bIsSceneDirty || bIsSwapChainInvalidated ? -1 : m_CommandsManager->DrawFrame(m_BufferManager->GetSwapChain());
        if (!m_SharedDeviceProperties.IsValid() || ImageIndice < 0)
        {
            if (!bIsSwapChainInvalidated)
            {
                m_CommandsManager->DestroySynchronizationObjects();
                m_BufferManager->DestroyResources(false);
                bIsSwapChainInvalidated = true;
            }

            m_SharedDeviceProperties = m_DeviceManager->GetPreferredProperties(Window);
            if (!m_SharedDeviceProperties.IsValid())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return;
            }

            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Refreshing device properties & capabilities...";
            
            m_BufferManager->CreateSwapChain(m_SharedDeviceProperties.PreferredFormat, m_SharedDeviceProperties.PreferredMode, m_SharedDeviceProperties.PreferredExtent, m_SharedDeviceProperties.Capabilities);
            bIsSwapChainInvalidated = false;
            m_BufferManager->CreateDepthResources(m_SharedDeviceProperties.PreferredDepthFormat, m_SharedDeviceProperties.PreferredExtent, m_DeviceManager->GetGraphicsQueue(), m_DeviceManager->GetGraphicsQueueFamilyIndex());
            m_BufferManager->CreateFrameBuffers(m_PipelineManager->GetRenderPass(), m_SharedDeviceProperties.PreferredExtent);
            m_CommandsManager->CreateSynchronizationObjects();

            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Buffers updated, starting to draw frames with new surface properties";
        }
        else
        {
            m_BufferManager->UpdateUniformBuffers(m_CommandsManager->GetCurrentFrameIndex(), m_SharedDeviceProperties.PreferredExtent);

            const VulkanCommandsManager::BufferRecordParameters Parameters = GetBufferRecordParameters(ImageIndice);
            m_CommandsManager->RecordCommandBuffers(Parameters);
            m_CommandsManager->SubmitCommandBuffers(m_DeviceManager->GetGraphicsQueue());
            m_CommandsManager->PresentFrame(m_DeviceManager->GetGraphicsQueue(), m_BufferManager->GetSwapChain(), ImageIndice);
        }
    }

    bool IsInitialized() const
    {
        return m_DeviceManager && m_DeviceManager->IsInitialized()
            && m_BufferManager && m_BufferManager->IsInitialized()
            && m_CommandsManager && m_CommandsManager->IsInitialized()
            && m_ShaderManager
            && m_Instance != VK_NULL_HANDLE
            && m_Surface != VK_NULL_HANDLE;
    }

    void LoadScene(const std::string_view ModelPath, const std::string_view TexturePath)
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

        m_BufferManager->LoadScene(ModelPath);
        m_BufferManager->CreateSwapChain(m_SharedDeviceProperties.PreferredFormat, m_SharedDeviceProperties.PreferredMode, m_SharedDeviceProperties.PreferredExtent, m_SharedDeviceProperties.Capabilities);
        bIsSwapChainInvalidated = false;
        m_BufferManager->CreateDepthResources(m_SharedDeviceProperties.PreferredDepthFormat, m_SharedDeviceProperties.PreferredExtent, m_DeviceManager->GetGraphicsQueue(), m_DeviceManager->GetGraphicsQueueFamilyIndex());
        
        m_PipelineManager->CreateRenderPass(m_SharedDeviceProperties.PreferredFormat.format, m_SharedDeviceProperties.PreferredDepthFormat);
        m_PipelineManager->CreateDescriptorSetLayout();
        m_PipelineManager->CreateGraphicsPipeline(m_DefaultShadersStageInfos);

        m_BufferManager->CreateFrameBuffers(m_PipelineManager->GetRenderPass(), m_SharedDeviceProperties.PreferredExtent);
        m_BufferManager->CreateVertexBuffers(m_DeviceManager->GetTransferQueue(), m_DeviceManager->GetTransferQueueFamilyIndex());
        m_BufferManager->CreateIndexBuffers(m_DeviceManager->GetTransferQueue(), m_DeviceManager->GetTransferQueueFamilyIndex());
        m_BufferManager->CreateUniformBuffers();

        VkImageView TextureView = VK_NULL_HANDLE;
        VkSampler TextureSampler = VK_NULL_HANDLE;
        m_BufferManager->CreateTextureImage(TexturePath, m_DeviceManager->GetGraphicsQueue(), m_DeviceManager->GetGraphicsQueueFamilyIndex(), TextureView, TextureSampler);
    
        m_PipelineManager->CreateDescriptorPool();
        m_PipelineManager->CreateDescriptorSets(m_BufferManager->GetUniformBuffers(), TextureView, TextureSampler);

        m_CommandsManager->CreateSynchronizationObjects();

        bIsSceneDirty = false;
        bHasLoadedScene = true;
    }

    void UnloadScene()
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

private:
    void CreateVulkanInstance()
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
            .enabledLayerCount = 0u,
        };

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

#ifdef _DEBUG
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Setting up debug messages";
        RENDERCORE_CHECK_VULKAN_RESULT(CreateDebugUtilsMessenger(m_Instance, &CreateDebugInfo, nullptr, &m_DebugMessenger));
#endif
    }

    void CreateVulkanSurface(GLFWwindow *const Window)
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
    }

    bool InitializeRenderCore(GLFWwindow *const Window)
    {
        if (!Window)
        {
            throw std::runtime_error("GLFW Window is invalid.");
        }

        InitializeDeviceManagement();
        m_SharedDeviceProperties = m_DeviceManager->GetPreferredProperties(Window);

        InitializeBufferManagement();
        InitializePipelineManagement();
        InitializeCommandsManagement();

        return IsInitialized();
    }

    void InitializeDeviceManagement()
    {
        if (!m_DeviceManager)
        {
            m_DeviceManager = std::make_unique<VulkanDeviceManager>(m_Instance, m_Surface);
        }

        m_DeviceManager->PickPhysicalDevice();
        m_DeviceManager->CreateLogicalDevice();
    }

    void InitializeBufferManagement()
    {
        if (!m_BufferManager)
        {
            m_BufferManager = std::make_unique<VulkanBufferManager>(m_DeviceManager->GetLogicalDevice(), m_Surface, m_DeviceManager->GetQueueFamilyIndices());
        }

        m_BufferManager->CreateMemoryAllocator(m_Instance, m_DeviceManager->GetLogicalDevice(), m_DeviceManager->GetPhysicalDevice());
    }

    void InitializePipelineManagement()
    {   
        if (!m_PipelineManager)
        {
            m_PipelineManager = std::make_unique<VulkanPipelineManager>(m_Instance, m_DeviceManager->GetLogicalDevice());
        }

        if (!m_ShaderManager)
        {
            m_ShaderManager = std::make_unique<VulkanShaderManager>(m_DeviceManager->GetLogicalDevice());
        }

        m_DefaultShadersStageInfos = CompileDefaultShaders();
    }

    void InitializeCommandsManagement()
    {
        if (!m_CommandsManager)
        {
            m_CommandsManager = std::make_unique<VulkanCommandsManager>(m_DeviceManager->GetLogicalDevice());
        }

        m_CommandsManager->SetGraphicsProcessingFamilyQueueIndex(m_DeviceManager->GetGraphicsQueueFamilyIndex());
    }

    std::vector<VkPipelineShaderStageCreateInfo> CompileDefaultShaders()
    {
        constexpr std::array<const char*, 1u> FragmentShaders = { DEBUG_SHADER_FRAG };
        constexpr std::array<const char*, 1u> VertexShaders = { DEBUG_SHADER_VERT };
        std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

        for (const char* const &FragmentShaderIter : FragmentShaders)
        {
            if (std::vector<std::uint32_t> FragmentShaderCode; m_ShaderManager->CompileOrLoadIfExists(FragmentShaderIter, FragmentShaderCode))
            {
                const VkShaderModule FragmentModule = m_ShaderManager->CreateModule(m_DeviceManager->GetLogicalDevice(), FragmentShaderCode, EShLangFragment);
                ShaderStages.push_back(m_ShaderManager->GetStageInfo(FragmentModule));
            }
        }

        for (const char* const &VertexShaderIter : VertexShaders)
        {
            if (std::vector<std::uint32_t> VertexShaderCode; m_ShaderManager->CompileOrLoadIfExists(VertexShaderIter, VertexShaderCode))
            {
                const VkShaderModule VertexModule = m_ShaderManager->CreateModule(m_DeviceManager->GetLogicalDevice(), VertexShaderCode, EShLangVertex);
                ShaderStages.push_back(m_ShaderManager->GetStageInfo(VertexModule));
            }
        }

        return ShaderStages;
    }

    VulkanCommandsManager::BufferRecordParameters GetBufferRecordParameters(const std::uint32_t ImageIndex) const{
        return {
            .RenderPass = m_PipelineManager->GetRenderPass(),
            .Pipeline = m_PipelineManager->GetPipeline(),
            .Extent = m_SharedDeviceProperties.PreferredExtent,
            .FrameBuffers = m_BufferManager->GetFrameBuffers(),
            .VertexBuffers = m_BufferManager->GetVertexBuffers(),
            .IndexBuffers = m_BufferManager->GetIndexBuffers(),
            .PipelineLayout = m_PipelineManager->GetPipelineLayout(),
            .DescriptorSets = m_PipelineManager->GetDescriptorSets(),
            .IndexCount = m_BufferManager->GetIndicesCount(),
            .ImageIndex = ImageIndex,
            .Offsets = {0u}
        };
    }

private:
    std::unique_ptr<VulkanDeviceManager> m_DeviceManager;
    std::unique_ptr<VulkanPipelineManager> m_PipelineManager;
    std::unique_ptr<VulkanBufferManager> m_BufferManager;
    std::unique_ptr<VulkanCommandsManager> m_CommandsManager;
    std::unique_ptr<VulkanShaderManager> m_ShaderManager;

    VkInstance m_Instance;
    VkSurfaceKHR m_Surface;
    DeviceProperties m_SharedDeviceProperties;
    std::vector<VkPipelineShaderStageCreateInfo> m_DefaultShadersStageInfos;
    bool bIsSceneDirty;
    bool bIsSwapChainInvalidated;
    bool bHasLoadedScene;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT m_DebugMessenger;
#endif
};

VulkanRender::VulkanRender()
    : m_Impl(std::make_unique<VulkanRender::Impl>())
{
}

VulkanRender::~VulkanRender()
{
    if (!IsInitialized())
    {
        return;
    }

    Shutdown();
}

bool VulkanRender::Initialize(GLFWwindow *const Window)
{
    if (IsInitialized())
    {
        return false;
    }

    return m_Impl->Initialize(Window);
}

void VulkanRender::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    m_Impl->Shutdown();
}

void VulkanRender::DrawFrame(GLFWwindow *const Window)
{
    if (!IsInitialized())
    {
        return;
    }

    m_Impl->DrawFrame(Window);
}

bool VulkanRender::IsInitialized() const
{
    return m_Impl && m_Impl->IsInitialized();
}

void VulkanRender::LoadScene(const std::string_view ModelPath, const std::string_view TexturePath)
{
    if (!IsInitialized())
    {
        return;
    }

    m_Impl->LoadScene(ModelPath, TexturePath);
}

void VulkanRender::UnloadScene()
{
    if (!IsInitialized())
    {
        return;
    }

    m_Impl->UnloadScene();
}
