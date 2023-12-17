// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <boost/log/trivial.hpp>
#include <filesystem>
#include <glm/ext.hpp>
#include <mutex>
#include <vector>

// Include vulkan before glfw
#ifndef VOLK_IMPLEMENTATION
#define VOLK_IMPLEMENTATION
#endif
#include <volk.h>

// Include glfw after vulkan
#include <GLFW/glfw3.h>

module RenderCore.Renderer;

import RenderCore.Management.DeviceManagement;
import RenderCore.Management.BufferManagement;
import RenderCore.Management.ShaderManagement;
import RenderCore.Management.CommandsManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Management.ImGuiManagement;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumHelpers;
import RenderCore.Utils.DebugHelpers;
import RenderCore.Utils.Constants;
import RenderCore.Subsystem.Rendering;
import Timer.ExecutionCounter;

using namespace RenderCore;

std::atomic_uint8_t g_RenderersCount {0U};
VkInstance g_Instance {VK_NULL_HANDLE};
Timer::Manager g_RenderTimerManager {};

#ifdef _DEBUG
VkDebugUtilsMessengerEXT g_DebugMessenger {VK_NULL_HANDLE};
#endif

bool CreateVulkanInstance()
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

    std::vector Layers(std::cbegin(g_RequiredInstanceLayers), std::cend(g_RequiredInstanceLayers));
    std::vector Extensions(std::cbegin(g_RequiredInstanceExtensions), std::cend(g_RequiredInstanceExtensions));

    // ReSharper disable once CppTooWideScopeInitStatement
    std::vector const GLFWExtensions = GetGLFWExtensions();

    for (std::string const& ExtensionIter: GLFWExtensions)
    {
        Extensions.push_back(std::data(ExtensionIter));
    }

#ifdef _DEBUG
    VkValidationFeaturesEXT const ValidationFeatures = GetInstanceValidationFeatures();
    CreateInfo.pNext                                 = &ValidationFeatures;

    Layers.insert(std::cend(Layers), std::cbegin(g_DebugInstanceLayers), std::cend(g_DebugInstanceLayers));
    Extensions.insert(std::cend(Extensions), std::cbegin(g_DebugInstanceExtensions), std::cend(g_DebugInstanceExtensions));

    VkDebugUtilsMessengerCreateInfoEXT CreateDebugInfo {};
    PopulateDebugInfo(CreateDebugInfo, nullptr);
#endif

    CreateInfo.enabledLayerCount   = static_cast<std::uint32_t>(std::size(Layers));
    CreateInfo.ppEnabledLayerNames = std::data(Layers);

    CreateInfo.enabledExtensionCount   = static_cast<std::uint32_t>(std::size(Extensions));
    CreateInfo.ppEnabledExtensionNames = std::data(Extensions);

    CheckVulkanResult(vkCreateInstance(&CreateInfo, nullptr, &g_Instance));
    volkLoadInstance(g_Instance);

#ifdef _DEBUG
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Setting up debug messages";
    CheckVulkanResult(CreateDebugUtilsMessenger(g_Instance, &CreateDebugInfo, nullptr, &g_DebugMessenger));
#endif

    return g_Instance != VK_NULL_HANDLE;
}

void Renderer::DrawFrame(GLFWwindow* const Window, Camera const& Camera)
{
    if (!IsInitialized())
    {
        return;
    }

    if (Window == nullptr)
    {
        throw std::runtime_error("Window is invalid");
    }

    static std::uint64_t LastTime   = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::uint64_t const CurrentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    double const DeltaTime          = static_cast<double>(CurrentTime - LastTime) / 1000.0;

    if (DeltaTime < m_FrameRateCap)
    {
        return;
    }

    if (DeltaTime > 0.F)
    {
        m_FrameTime = DeltaTime;
        LastTime    = CurrentTime;
    }

    RemoveInvalidObjects();

    constexpr RendererStateFlags InvalidStatesToRender = RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE
                                                         | RendererStateFlags::PENDING_RESOURCES_DESTRUCTION
                                                         | RendererStateFlags::PENDING_RESOURCES_CREATION
                                                         | RendererStateFlags::PENDING_PIPELINE_REFRESH;

    if (HasAnyFlag(m_StateFlags, InvalidStatesToRender))
    {
        if (HasFlag(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION))
        {
            DestroyCommandsSynchronizationObjects();
            m_BufferManager.DestroyBufferResources(false);
            m_PipelineManager.ReleaseDynamicPipelineResources();

            RemoveFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
            AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);
        }

        if (HasFlag(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION) && !HasFlag(m_StateFlags, RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE))
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Refreshing resources...";

            auto const SurfaceProperties   = GetSurfaceProperties(Window, m_BufferManager.GetSurface());
            auto const SurfaceCapabilities = GetSurfaceCapabilities(m_BufferManager.GetSurface());

            auto const QueueFamilyIndices = GetUniqueQueueFamilyIndicesU32();
            m_BufferManager.CreateSwapChain(SurfaceProperties, SurfaceCapabilities, QueueFamilyIndices);
            m_BufferManager.CreateDepthResources(SurfaceProperties, GetGraphicsQueue());
            CreateCommandsSynchronizationObjects();

            RemoveFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);
            AddFlags(m_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }

        if (HasFlag(m_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH))
        {
            m_PipelineManager.CreateDescriptorSetLayout();
            m_PipelineManager.CreateGraphicsPipeline();

            m_BufferManager.CreateFrameBuffers(m_PipelineManager.GetRenderPass());

            m_PipelineManager.CreateDescriptorPool(m_BufferManager.GetClampedNumAllocations());
            m_PipelineManager.CreateDescriptorSets(m_BufferManager.GetAllocatedObjects());

            RemoveFlags(m_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }
    }

    if (!HasAnyFlag(m_StateFlags, InvalidStatesToRender))
    {
        if (std::optional<std::int32_t> const ImageIndice = RequestImageIndex(Window);
            ImageIndice.has_value())
        {
            std::lock_guard Lock(m_ObjectsMutex);

            RecordCommandBuffers(GetGraphicsQueue().first, ImageIndice.value(), Camera, m_BufferManager, m_PipelineManager, GetObjects());
            SubmitCommandBuffers(GetGraphicsQueue().second);
            PresentFrame(GetGraphicsQueue().second, ImageIndice.value(), m_BufferManager.GetSwapChain());
        }
    }
}

std::optional<std::int32_t> Renderer::RequestImageIndex(GLFWwindow* const Window)
{
    std::optional<std::int32_t> const Output = RequestSwapChainImage(m_BufferManager.GetSwapChain());

    if (!Output.has_value())
    {
        AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);

        if (!HasFlag(m_StateFlags, RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE) && !GetSurfaceProperties(Window, m_BufferManager.GetSurface()).IsValid())
        {
            AddFlags(m_StateFlags, RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
            AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
        }
        else
        {
            RemoveFlags(m_StateFlags, RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
        }
    }
    else
    {
        RemoveFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }

    return Output;
}

void Renderer::Tick()
{
    if (!IsInitialized())
    {
        return;
    }

    static std::uint64_t LastTime   = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::uint64_t const CurrentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    double const DeltaTime          = static_cast<double>(CurrentTime - LastTime) / 1000.0;

    if (DeltaTime <= 0.F)
    {
        return;
    }

    m_DeltaTime = DeltaTime;
    LastTime    = CurrentTime;

    if (m_ObjectsMutex.try_lock())
    {
        for (std::shared_ptr<Object> const& ObjectIter: m_Objects)
        {
            if (ObjectIter && !ObjectIter->IsPendingDestroy())
            {
                ObjectIter->Tick(DeltaTime);
            }
        }

        m_ObjectsMutex.unlock();
    }
}

void Renderer::RemoveInvalidObjects()
{
    if (std::empty(m_Objects))
    {
        return;
    }

    if (m_ObjectsMutex.try_lock())
    {
        std::vector<std::uint32_t> LoadedIDs {};
        for (std::shared_ptr<Object> const& ObjectIter: m_Objects)
        {
            if (ObjectIter && ObjectIter->IsPendingDestroy())
            {
                LoadedIDs.push_back(ObjectIter->GetID());
            }
        }

        m_ObjectsMutex.unlock();

        if (!std::empty(LoadedIDs))
        {
            UnloadScene(LoadedIDs);
        }
    }
}

bool Renderer::Initialize(GLFWwindow* const Window)
{
    if (IsInitialized())
    {
        return false;
    }

    RenderingSubsystem::Get().RegisterRenderer(Window, *this);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing vulkan renderer";

    CheckVulkanResult(volkInitialize());

    if ((++g_RenderersCount == 1U || g_Instance == VK_NULL_HANDLE) && !CreateVulkanInstance())
    {
        throw std::runtime_error("Failed to create vulkan instance");
    }

    m_BufferManager.CreateVulkanSurface(Window);
    InitializeDevice(m_BufferManager.GetSurface());
    volkLoadDevice(GetLogicalDevice());

    m_BufferManager.CreateMemoryAllocator(GetPhysicalDevice());
    auto const _                 = CompileDefaultShaders();
    auto const SurfaceProperties = GetSurfaceProperties(Window, m_BufferManager.GetSurface());
    m_PipelineManager.CreateRenderPass(SurfaceProperties);
    InitializeImGui(Window, m_PipelineManager);

    AddFlags(m_StateFlags, RendererStateFlags::INITIALIZED);
    AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);

    return SurfaceProperties.IsValid() && IsInitialized();
}

void Renderer::Shutdown(GLFWwindow* const Window)
{
    if (!IsInitialized())
    {
        return;
    }

    RenderingSubsystem::Get().RegisterRenderer(Window, *this);

    RemoveFlags(m_StateFlags, RendererStateFlags::INITIALIZED);

    bool const DestroyInstance = --g_RenderersCount == 0U;

    if (DestroyInstance)
    {
        ReleaseImGuiResources();
        ReleaseShaderResources();
        ReleaseCommandsResources();
    }

    m_BufferManager.ReleaseBufferResources();
    m_PipelineManager.ReleasePipelineResources();

    if (DestroyInstance)
    {
        ReleaseDeviceResources();
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan renderer";

#ifdef _DEBUG
    if (g_DebugMessenger != VK_NULL_HANDLE)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan debug messenger";

        DestroyDebugUtilsMessenger(g_Instance, g_DebugMessenger, nullptr);
        g_DebugMessenger = VK_NULL_HANDLE;
    }
#endif

    vkDestroyInstance(g_Instance, nullptr);
    g_Instance = VK_NULL_HANDLE;

    m_Objects.clear();
}

bool Renderer::IsInitialized() const
{
    return HasFlag(m_StateFlags, RendererStateFlags::INITIALIZED);
}

void Renderer::AddStateFlag(RendererStateFlags const Flag)
{
    AddFlags(m_StateFlags, Flag);
}

void Renderer::RemoveStateFlag(RendererStateFlags const Flag)
{
    RemoveFlags(m_StateFlags, Flag);
}

bool Renderer::HasStateFlag(RendererStateFlags const Flag) const
{
    return HasFlag(m_StateFlags, Flag);
}

std::vector<std::uint32_t> Renderer::LoadScene(std::string const& ObjectPath)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    if (!IsInitialized())
    {
        return {};
    }

    if (!std::filesystem::exists(ObjectPath))
    {
        throw std::runtime_error("Object path is invalid");
    }

    std::lock_guard Lock(m_ObjectsMutex);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading scene...";

    std::vector<Object> const LoadedObjects = m_BufferManager.AllocateScene(ObjectPath, GetGraphicsQueue());
    std::vector<std::uint32_t> Output;
    Output.reserve(std::size(LoadedObjects));

    for (Object const& ObjectIter: LoadedObjects)
    {
        m_Objects.emplace_back(std::make_shared<Object>(ObjectIter));
        Output.push_back(ObjectIter.GetID());
    }

    AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
    AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);

    return Output;
}

void Renderer::UnloadScene(std::vector<std::uint32_t> const& ObjectIDs)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    if (!IsInitialized())
    {
        return;
    }

    std::lock_guard Lock(m_ObjectsMutex);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Unloading scene...";

    m_BufferManager.ReleaseScene(ObjectIDs);

    for (std::uint32_t const ObjectID: ObjectIDs)
    {
        std::erase_if(m_Objects, [ObjectID](std::shared_ptr<Object> const& ObjectIter) {
            return !ObjectIter || ObjectIter->GetID() == ObjectID;
        });
    }

    AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
}

void Renderer::UnloadAllScenes()
{
    std::vector<std::uint32_t> LoadedIDs {};
    for (std::shared_ptr<Object> const& ObjectIter: m_Objects)
    {
        if (ObjectIter)
        {
            LoadedIDs.push_back(ObjectIter->GetID());
        }
    }
    UnloadScene(LoadedIDs);
    m_Objects.clear();
}

Timer::Manager& Renderer::GetRenderTimerManager()
{
    return g_RenderTimerManager;
}

double Renderer::GetDeltaTime() const
{
    return m_DeltaTime;
}

double Renderer::GetFrameTime() const
{
    return m_FrameTime;
}

void Renderer::SetFrameRateCap(double const FrameRateCap)
{
    if (FrameRateCap > 0.0)
    {
        m_FrameRateCap = 1.0 / FrameRateCap;
    }
}

double Renderer::GetFrameRateCap() const
{
    return m_FrameRateCap;
}

Camera const& Renderer::GetCamera() const
{
    return m_Camera;
}

Camera& Renderer::GetMutableCamera()
{
    return m_Camera;
}

std::vector<std::shared_ptr<Object>> const& Renderer::GetObjects() const
{
    return m_Objects;
}

std::shared_ptr<Object> Renderer::GetObjectByID(std::uint32_t const ObjectID) const
{
    return *std::ranges::find_if(m_Objects, [ObjectID](std::shared_ptr<Object> const& ObjectIter) {
        return ObjectIter && ObjectIter->GetID() == ObjectID;
    });
}

std::uint32_t Renderer::GetNumObjects() const
{
    return std::size(m_Objects);
}