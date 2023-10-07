// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <boost/log/trivial.hpp>
#include <glm/ext.hpp>

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
import RenderCore.Management.CommandsManagement;
import RenderCore.Management.BufferManagement;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Management.ShaderManagement;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.DebugHelpers;
import RenderCore.Utils.EnumHelpers;

using namespace RenderCore;

enum class EngineCoreStateFlags : std::uint8_t
{
    NONE                             = 0,
    INITIALIZED                      = 1 << 0,
    PENDING_DEVICE_PROPERTIES_UPDATE = 1 << 1,
    PENDING_RESOURCES_DESTRUCTION    = 1 << 2,
    PENDING_RESOURCES_CREATION       = 1 << 3,
    PENDING_PIPELINE_REFRESH         = 1 << 4,
};

VkInstance g_Instance {VK_NULL_HANDLE};
VkSurfaceKHR g_Surface {VK_NULL_HANDLE};
EngineCoreStateFlags g_StateFlags {EngineCoreStateFlags::NONE};
glm::mat4 g_CameraMatrix {lookAt(glm::vec3(2.F, 2.F, 2.F), glm::vec3(0.F, 0.F, 0.F), glm::vec3(0.F, 0.F, 1.F))};
std::uint32_t g_ObjectID {0U};

#ifdef _DEBUG
VkDebugUtilsMessengerEXT g_DebugMessenger {VK_NULL_HANDLE};
#endif

std::optional<std::int32_t> TryRequestDrawImage()
{
    if (!GetDeviceProperties().IsValid())
    {
        AddFlags(g_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
        AddFlags(g_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);

        return std::nullopt;
    }
    RemoveFlags(g_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);

    std::optional<std::int32_t> const Output = RequestSwapChainImage();

    if (!Output.has_value())
    {
        AddFlags(g_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }
    else
    {
        RemoveFlags(g_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }

    return Output;
}

std::vector<VkPipelineShaderStageCreateInfo> CompileDefaultShaders()
{
    constexpr std::array FragmentShaders {
            DEBUG_SHADER_FRAG};
    constexpr std::array VertexShaders {
            DEBUG_SHADER_VERT};

    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

    VkDevice const& VulkanLogicalDevice = GetLogicalDevice();

    for (char const* const& FragmentShaderIter: FragmentShaders)
    {
        if (std::vector<std::uint32_t> FragmentShaderCode;
            CompileOrLoadIfExists(FragmentShaderIter, FragmentShaderCode))
        {
            VkShaderModule const FragmentModule = CreateModule(VulkanLogicalDevice, FragmentShaderCode, EShLangFragment);
            ShaderStages.push_back(GetStageInfo(FragmentModule));
        }
    }

    for (char const* const& VertexShaderIter: VertexShaders)
    {
        if (std::vector<std::uint32_t> VertexShaderCode;
            CompileOrLoadIfExists(VertexShaderIter, VertexShaderCode))
        {
            VkShaderModule const VertexModule = CreateModule(VulkanLogicalDevice, VertexShaderCode, EShLangVertex);
            ShaderStages.push_back(GetStageInfo(VertexModule));
        }
    }

    return ShaderStages;
}

void CreateVulkanInstance()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan instance";

    constexpr VkApplicationInfo AppInfo {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName   = "VulkanApp",
            .applicationVersion = VK_MAKE_VERSION(1U, 0U, 0U),
            .pEngineName        = "No Engine",
            .engineVersion      = VK_MAKE_VERSION(1U, 0U, 0U),
            .apiVersion         = VK_API_VERSION_1_3};

    VkInstanceCreateInfo CreateInfo {
            .sType             = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext             = nullptr,
            .pApplicationInfo  = &AppInfo,
            .enabledLayerCount = 0U};

    std::vector Layers(g_RequiredInstanceLayers.begin(), g_RequiredInstanceLayers.end());
    std::vector<char const*> Extensions = GetGLFWExtensions();
    Extensions.insert(Extensions.end(), g_RequiredInstanceExtensions.begin(), g_RequiredInstanceExtensions.end());

#ifdef _DEBUG
    VkValidationFeaturesEXT const ValidationFeatures = GetInstanceValidationFeatures();
    CreateInfo.pNext                                 = &ValidationFeatures;

    Layers.insert(Layers.end(), g_DebugInstanceLayers.begin(), g_DebugInstanceLayers.end());
    Extensions.insert(Extensions.end(), g_DebugInstanceExtensions.begin(), g_DebugInstanceExtensions.end());

    VkDebugUtilsMessengerCreateInfoEXT CreateDebugInfo {};
    PopulateDebugInfo(CreateDebugInfo, nullptr);
#endif

    CreateInfo.enabledLayerCount   = static_cast<std::uint32_t>(Layers.size());
    CreateInfo.ppEnabledLayerNames = Layers.data();

    CreateInfo.enabledExtensionCount   = static_cast<std::uint32_t>(Extensions.size());
    CreateInfo.ppEnabledExtensionNames = Extensions.data();

    CheckVulkanResult(vkCreateInstance(&CreateInfo, nullptr, &g_Instance));
    volkLoadInstance(g_Instance);

#ifdef _DEBUG
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Setting up debug messages";
    CheckVulkanResult(CreateDebugUtilsMessenger(g_Instance, &CreateDebugInfo, nullptr, &g_DebugMessenger));
#endif
}

void CreateVulkanSurface(GLFWwindow* const Window)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan surface";

    if (g_Instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan instance is invalid.");
    }

    CheckVulkanResult(glfwCreateWindowSurface(g_Instance, Window, nullptr, &g_Surface));
}

void InitializeRenderCore()
{
    PickPhysicalDevice();
    CreateLogicalDevice();
    volkLoadDevice(GetLogicalDevice());

    CreateMemoryAllocator();
    CompileDefaultShaders();

    AddFlags(g_StateFlags, EngineCoreStateFlags::INITIALIZED);
    AddFlags(g_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION);
}

void RenderCore::InitializeEngine(GLFWwindow* const Window)
{
    if (IsEngineInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing vulkan engine core";

    CheckVulkanResult(volkInitialize());

#ifdef _DEBUG
    ListAvailableInstanceLayers();

    for (char const* const& RequiredLayerIter: g_RequiredInstanceLayers)
    {
        ListAvailableInstanceLayerExtensions(RequiredLayerIter);
    }

    for (char const* const& DebugLayerIter: g_DebugInstanceLayers)
    {
        ListAvailableInstanceLayerExtensions(DebugLayerIter);
    }
#endif

    CreateVulkanInstance();
    CreateVulkanSurface(Window);
    InitializeRenderCore();
}

void RenderCore::ShutdownEngine()
{
    if (!IsEngineInitialized())
    {
        return;
    }

    RemoveFlags(g_StateFlags, EngineCoreStateFlags::INITIALIZED);

    ReleaseShaderResources();
    ReleaseCommandsResources();
    ReleaseBufferResources();
    ReleasePipelineResources();
    ReleaseDeviceResources();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan engine core";

#ifdef _DEBUG
    if (g_DebugMessenger != VK_NULL_HANDLE)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan debug messenger";

        DestroyDebugUtilsMessenger(g_Instance, g_DebugMessenger, nullptr);
        g_DebugMessenger = VK_NULL_HANDLE;
    }
#endif

    vkDestroySurfaceKHR(g_Instance, g_Surface, nullptr);
    g_Surface = VK_NULL_HANDLE;

    vkDestroyInstance(g_Instance, nullptr);
    g_Instance = VK_NULL_HANDLE;
}

void RenderCore::DrawFrame(GLFWwindow* const Window)
{
    if (!IsEngineInitialized())
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

    if (HasAnyFlag(g_StateFlags, InvalidStatesToRender))
    {
        if (HasFlag(g_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION))
        {
            DestroyCommandsSynchronizationObjects();
            DestroyBufferResources(false);
            ReleasePipelineResources();

            RemoveFlags(g_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
            AddFlags(g_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION);
        }

        if (HasFlag(g_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE))
        {
            if (UpdateDeviceProperties(Window))
            {
                BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Device properties updated, starting to draw frames with new properties";
                RemoveFlags(g_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
            }
        }

        if (HasFlag(g_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION) && !HasFlag(g_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE))
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Refreshing resources...";
            CreateSwapChain();
            CreateDepthResources();
            CreateCommandsSynchronizationObjects();

            RemoveFlags(g_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION);
            AddFlags(g_StateFlags, EngineCoreStateFlags::PENDING_PIPELINE_REFRESH);
        }

        if (HasFlag(g_StateFlags, EngineCoreStateFlags::PENDING_PIPELINE_REFRESH))
        {
            CreateRenderPass();
            CreateDescriptorSetLayout();
            CreateGraphicsPipeline();

            CreateFrameBuffers();

            CreateDescriptorPool();
            CreateDescriptorSets();

            RemoveFlags(g_StateFlags, EngineCoreStateFlags::PENDING_PIPELINE_REFRESH);
        }
    }

    if (!HasAnyFlag(g_StateFlags, InvalidStatesToRender))
    {
        if (std::optional<std::int32_t> const ImageIndice = TryRequestDrawImage();
            ImageIndice.has_value())
        {
            RecordCommandBuffers(ImageIndice.value());
            SubmitCommandBuffers();
            PresentFrame(ImageIndice.value());
        }
    }
}

bool RenderCore::IsEngineInitialized()
{
    return HasFlag(g_StateFlags, EngineCoreStateFlags::INITIALIZED);
}

void RenderCore::LoadScene(std::string_view const ModelPath, std::string_view const TexturePath)
{
    if (!IsEngineInitialized())
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

    g_ObjectID = LoadObject(ModelPath, TexturePath);

    AddFlags(g_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
    AddFlags(g_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION);
}

void RenderCore::UnloadScene()
{
    if (!IsEngineInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Unloading scene...";

    UnLoadObject(g_ObjectID);

    AddFlags(g_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
}

VkInstance& RenderCore::GetInstance()
{
    return g_Instance;
}

VkSurfaceKHR& RenderCore::GetSurface()
{
    return g_Surface;
}

glm::mat4 RenderCore::GetCameraMatrix()
{
    return g_CameraMatrix;
}

void RenderCore::SetCameraMatrix(glm::mat4 const& Value)
{
    g_CameraMatrix = Value;
}