// Copyright Notice: [...]

#include "VulkanRender.h"
#include "VulkanDeviceManager.h"
#include "VulkanPipelineManager.h"
#include "VulkanBufferManager.h"
#include "VulkanCommandsManager.h"
#include "VulkanShaderCompiler.h"
#include "VulkanDebugHelpers.h"
#include "VulkanConstants.h"
#include "RenderCoreHelpers.h"
#include <boost/log/trivial.hpp>
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
        , m_ShaderCompiler(nullptr)
        , m_Instance(VK_NULL_HANDLE)
        , m_Surface(VK_NULL_HANDLE)
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

    bool InitializeDeviceManager(GLFWwindow* const Window)
    {
        if (!Window)
        {
            throw std::runtime_error("GLFW Window is invalid.");
        }

        if (m_DeviceManager = std::make_unique<VulkanDeviceManager>(m_Instance, m_Surface);
            m_DeviceManager)
        {
            m_DeviceManager->PickPhysicalDevice();
            m_DeviceManager->CreateLogicalDevice();

            m_ShaderCompiler = std::make_unique<VulkanShaderCompiler>(m_DeviceManager->GetLogicalDevice());
        }

        return m_DeviceManager != nullptr;
    }

    bool InitializeBufferManager(GLFWwindow* const Window)
    {
        if (m_BufferManager = std::make_unique<VulkanBufferManager>(m_DeviceManager->GetLogicalDevice(), m_Surface, m_DeviceManager->GetQueueFamilyIndices());
            m_BufferManager)
        {
            VkSurfaceFormatKHR PreferredFormat;
            VkPresentModeKHR PreferredMode;
            VkExtent2D PreferredExtent;
            VkSurfaceCapabilitiesKHR Capabilities;
            m_DeviceManager->GetSwapChainPreferredProperties(Window, PreferredFormat, PreferredMode, PreferredExtent, Capabilities);
            m_BufferManager->InitializeSwapChain(PreferredFormat, PreferredMode, PreferredExtent, Capabilities);
        }

        return m_BufferManager != nullptr;
    }

    bool InitializePipelineManager(GLFWwindow* const Window)
    {
        if (m_PipelineManager = std::make_unique<VulkanPipelineManager>(m_Instance, m_DeviceManager->GetLogicalDevice());
            m_PipelineManager && m_ShaderCompiler)
        {
            VkSurfaceFormatKHR PreferredFormat;
            VkPresentModeKHR PreferredMode;
            VkExtent2D PreferredExtent;
            VkSurfaceCapabilitiesKHR Capabilities;
            m_DeviceManager->GetSwapChainPreferredProperties(Window, PreferredFormat, PreferredMode, PreferredExtent, Capabilities);
            m_PipelineManager->CreateRenderPass(PreferredFormat.format);

            std::vector<std::uint32_t> FragmentShaderCode;
            m_ShaderCompiler->Compile(DEBUG_SHADER_FRAG, FragmentShaderCode);
            const VkShaderModule FragmentModule = m_ShaderCompiler->CreateModule(m_DeviceManager->GetLogicalDevice(), FragmentShaderCode, EShLangFragment);

            std::vector<std::uint32_t> VertexShaderCode;
            m_ShaderCompiler->Compile(DEBUG_SHADER_VERT, VertexShaderCode);
            const VkShaderModule VertexModule = m_ShaderCompiler->CreateModule(m_DeviceManager->GetLogicalDevice(), VertexShaderCode, EShLangVertex);

            m_PipelineManager->CreateGraphicsPipeline({ m_ShaderCompiler->GetStageInfo(FragmentModule), m_ShaderCompiler->GetStageInfo(VertexModule) });
        }

        return m_PipelineManager != nullptr;
    }

    void Shutdown()
    {
        if (!IsInitialized())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan render";

        m_ShaderCompiler->Shutdown();
        m_BufferManager->Shutdown();
        m_PipelineManager->Shutdown();
        // m_CommandsManager->Shutdown();
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

    bool IsInitialized() const
    {
        return m_DeviceManager && m_DeviceManager->IsInitialized()
            && m_PipelineManager && m_PipelineManager->IsInitialized()
            && m_BufferManager && m_BufferManager->IsInitialized()
            // && m_CommandsManager && m_CommandsManager->IsInitialized()
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

    std::unique_ptr<VulkanDeviceManager> m_DeviceManager;
    std::unique_ptr<VulkanPipelineManager> m_PipelineManager;
    std::unique_ptr<VulkanBufferManager> m_BufferManager;
    std::unique_ptr<VulkanCommandsManager> m_CommandsManager;
    std::unique_ptr<VulkanShaderCompiler> m_ShaderCompiler;

    VkInstance m_Instance;
    VkSurfaceKHR m_Surface;

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
        m_Impl->CreateVulkanInstance();
        m_Impl->CreateVulkanSurface(Window);

        return m_Impl->InitializeDeviceManager(Window)
            && m_Impl->InitializeBufferManager(Window)
            && m_Impl->InitializePipelineManager(Window);
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

bool VulkanRender::IsInitialized() const
{
    return m_Impl && m_Impl->IsInitialized();;
}