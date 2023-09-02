// Copyright Notice: [...]

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

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

using namespace RenderCore;

class VulkanRender::Impl
{
public:
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    Impl()
        : m_DeviceManager(nullptr)
        , m_PipelineManager(nullptr)
        , m_BufferManager(nullptr)
        , m_CommandsManager(nullptr)
        , m_ShaderManager(nullptr)
        , m_Instance(VK_NULL_HANDLE)
        , m_Surface(VK_NULL_HANDLE)
        , m_SharedDeviceProperties()
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

    bool Initialize(GLFWwindow* const Window)
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

        CreateVulkanInstance();
        CreateVulkanSurface(Window);

        return InitializeDeviceManagement(Window)
            && InitializeBufferManagement(Window)
            && InitializePipelineManagement(Window)
            && InitializeCommandsManagement(Window);
    }

    void Shutdown()
    {
        if (!IsInitialized())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan render";

        if (vkDeviceWaitIdle(m_DeviceManager->GetLogicalDevice()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to wait for vulkan device idle");
        }

        m_ShaderManager->Shutdown();
        m_CommandsManager->Shutdown({ m_DeviceManager->GetGraphicsQueue(), m_DeviceManager ->GetPresentationQueue()});
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

    void DrawFrame(GLFWwindow* const Window)
    {
        if (!IsInitialized())
        {
            return;
        }

        const std::vector<std::uint32_t> ImageIndexes = m_CommandsManager->DrawFrame({ m_BufferManager->GetSwapChain() });
        if (ImageIndexes.empty())
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Refreshing device properties & capabilities...";
            m_SharedDeviceProperties = m_DeviceManager->GetPreferredProperties(Window);
            m_BufferManager->CreateSwapChain(m_SharedDeviceProperties.PreferredFormat, m_SharedDeviceProperties.PreferredMode, m_SharedDeviceProperties.PreferredExtent, m_SharedDeviceProperties.Capabilities);
            m_BufferManager->CreateFrameBuffers(m_PipelineManager->GetRenderPass(), m_SharedDeviceProperties.PreferredExtent);
            m_BufferManager->CreateVertexBuffers();

            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Buffers updated, starting to draw frames with new surface properties";
        }
        else
        {
            m_CommandsManager->RecordCommandBuffers(m_PipelineManager->GetRenderPass(), m_PipelineManager->GetPipeline(), m_SharedDeviceProperties.PreferredExtent, m_BufferManager->GetFrameBuffers(), m_BufferManager->GetVertexBuffers(), { 0u });
            m_CommandsManager->SubmitCommandBuffers(m_DeviceManager->GetGraphicsQueue());
            m_CommandsManager->PresentFrame(m_DeviceManager->GetPresentationQueue(), { m_BufferManager->GetSwapChain() }, ImageIndexes);
        }
    }

    bool IsInitialized() const
    {
        return m_DeviceManager && m_DeviceManager->IsInitialized()
            && m_PipelineManager && m_PipelineManager->IsInitialized()
            && m_BufferManager && m_BufferManager->IsInitialized()
            && m_CommandsManager && m_CommandsManager->IsInitialized()
            && m_ShaderManager
            && m_Instance != VK_NULL_HANDLE
            && m_Surface != VK_NULL_HANDLE;
    }

    bool SupportsValidationLayer() const
    {
        const std::set<std::string> RequiredLayers(g_ValidationLayers.begin(), g_ValidationLayers.end());
        const std::vector<VkLayerProperties> AvailableLayers = GetAvailableValidationLayers();

        return std::find_if(AvailableLayers.begin(),
            AvailableLayers.end(),
            [RequiredLayers](const VkLayerProperties& Item)
            {
                return std::find(RequiredLayers.begin(),
                RequiredLayers.end(),
                Item.layerName) != RequiredLayers.end();
            }
        ) != AvailableLayers.end();
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
            .apiVersion = VK_API_VERSION_1_0
        };

        VkInstanceCreateInfo CreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .pApplicationInfo = &AppInfo,
            .enabledLayerCount = 0u,
        };

        std::vector<const char*> Extensions = GetGLFWExtensions();

#ifdef _DEBUG
        const bool bSupportsValidationLayer = SupportsValidationLayer();

        VkDebugUtilsMessengerCreateInfoEXT CreateDebugInfo{};
        PopulateDebugInfo(CreateDebugInfo);

        if (bSupportsValidationLayer)
        {
            Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Activating validation layers in vulkan instance";
            for (const char* const& ValidationLayerIter : g_ValidationLayers)
            {
                BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Including Layer: " << ValidationLayerIter;
            }

            CreateInfo.enabledLayerCount = static_cast<std::uint32_t>(g_ValidationLayers.size());
            CreateInfo.ppEnabledLayerNames = g_ValidationLayers.data();

            CreateInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&CreateDebugInfo);
        }
#endif

        CreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(Extensions.size());
        CreateInfo.ppEnabledExtensionNames = Extensions.data();

        if (vkCreateInstance(&CreateInfo, nullptr, &m_Instance) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan instance.");
        }

#ifdef _DEBUG
        if (bSupportsValidationLayer)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Setting up debug messages";

            if (CreateDebugUtilsMessenger(m_Instance, &CreateDebugInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to set up debug messenger!");
            }
        }
#endif
    }

    void CreateVulkanSurface(GLFWwindow* const Window)
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

        if (glfwCreateWindowSurface(m_Instance, Window, nullptr, &m_Surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface.");
        }
    }

    bool InitializeDeviceManagement(GLFWwindow* const Window)
    {
        if (!Window)
        {
            throw std::runtime_error("GLFW Window is invalid.");
        }

        if (m_DeviceManager = std::make_unique<VulkanDeviceManager>(m_Instance, m_Surface))
        {
            m_DeviceManager->PickPhysicalDevice();
            m_DeviceManager->CreateLogicalDevice();

            m_ShaderManager = std::make_unique<VulkanShaderManager>(m_DeviceManager->GetLogicalDevice());
        }

        return m_DeviceManager != nullptr;
    }

    bool InitializeBufferManagement(GLFWwindow* const Window)
    {
        if (m_BufferManager = std::make_unique<VulkanBufferManager>(m_DeviceManager->GetLogicalDevice(), m_Surface, m_DeviceManager->GetQueueFamilyIndices()))
        {
            m_SharedDeviceProperties = m_DeviceManager->GetPreferredProperties(Window);
            m_BufferManager->CreateSwapChain(m_SharedDeviceProperties.PreferredFormat, m_SharedDeviceProperties.PreferredMode, m_SharedDeviceProperties.PreferredExtent, m_SharedDeviceProperties.Capabilities);
        }

        return m_BufferManager != nullptr;
    }

    bool InitializePipelineManagement(GLFWwindow* const Window)
    {
        if (m_PipelineManager = std::make_unique<VulkanPipelineManager>(m_Instance, m_DeviceManager->GetLogicalDevice()))
        {
            m_PipelineManager->CreateRenderPass(m_SharedDeviceProperties.PreferredFormat.format);
            m_BufferManager->CreateFrameBuffers(m_PipelineManager->GetRenderPass(), m_SharedDeviceProperties.PreferredExtent);

            std::vector<std::uint32_t> FragmentShaderCode;
            m_ShaderManager->Compile(DEBUG_SHADER_FRAG, FragmentShaderCode);
            const VkShaderModule FragmentModule = m_ShaderManager->CreateModule(m_DeviceManager->GetLogicalDevice(), FragmentShaderCode, EShLangFragment);

            std::vector<std::uint32_t> VertexShaderCode;
            m_ShaderManager->Compile(DEBUG_SHADER_VERT, VertexShaderCode);
            const VkShaderModule VertexModule = m_ShaderManager->CreateModule(m_DeviceManager->GetLogicalDevice(), VertexShaderCode, EShLangVertex);

            m_PipelineManager->CreateGraphicsPipeline({ m_ShaderManager->GetStageInfo(FragmentModule), m_ShaderManager->GetStageInfo(VertexModule) }, m_SharedDeviceProperties.PreferredExtent);
        }

        return m_PipelineManager != nullptr;
    }

    bool InitializeCommandsManagement(GLFWwindow* const Window)
    {
        if (m_CommandsManager = std::make_unique<VulkanCommandsManager>(m_DeviceManager->GetLogicalDevice()))
        {
            m_CommandsManager->CreateCommandPool(m_BufferManager->GetFrameBuffers(), m_DeviceManager->GetGraphicsQueueFamilyIndex());
            m_CommandsManager->RecordCommandBuffers(m_PipelineManager->GetRenderPass(), m_PipelineManager->GetPipeline(), m_SharedDeviceProperties.PreferredExtent, m_BufferManager->GetFrameBuffers(), m_BufferManager->GetVertexBuffers(), { 0u });
            m_CommandsManager->CreateSynchronizationObjects();
        }

        return m_CommandsManager != nullptr;
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

bool VulkanRender::Initialize(GLFWwindow* const Window)
{
    if (IsInitialized())
    {
        return false;
    }

    try
    {
        return m_Impl->Initialize(Window);
    }
    catch (const std::exception& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }

    return false;
}

void VulkanRender::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    try
    {
        m_Impl->Shutdown();
    }
    catch (const std::exception& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}

void VulkanRender::DrawFrame(GLFWwindow* const Window)
{
    if (!IsInitialized())
    {
        return;
    }

    try
    {
        m_Impl->DrawFrame(Window);
    }
    catch (const std::exception& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}

bool VulkanRender::IsInitialized() const
{
    return m_Impl && m_Impl->IsInitialized();;
}