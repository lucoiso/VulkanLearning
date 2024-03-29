// Author: Lucas Vilas-Boas
// Year : 2024
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
#include <Volk/volk.h>

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
import RuntimeInfo.Manager;

using namespace RenderCore;

VkInstance     g_Instance {VK_NULL_HANDLE};
Timer::Manager g_RenderTimerManager {};

constexpr RendererStateFlags g_InvalidStatesToRender = RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE | RendererStateFlags::PENDING_RESOURCES_DESTRUCTION
    | RendererStateFlags::PENDING_RESOURCES_CREATION | RendererStateFlags::PENDING_PIPELINE_REFRESH;

#ifdef _DEBUG
VkDebugUtilsMessengerEXT g_DebugMessenger {VK_NULL_HANDLE};
#endif

bool CreateVulkanInstance()
{
    RuntimeInfo::Manager::Get().PushCallstack();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan instance";

    constexpr VkApplicationInfo AppInfo {.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                         .pApplicationName   = "VulkanApp",
                                         .applicationVersion = VK_MAKE_VERSION(1U, 0U, 0U),
                                         .pEngineName        = "No Engine",
                                         .engineVersion      = VK_MAKE_VERSION(1U, 0U, 0U),
                                         .apiVersion         = VK_API_VERSION_1_3};

    VkInstanceCreateInfo CreateInfo {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pNext = nullptr, .pApplicationInfo = &AppInfo, .enabledLayerCount = 0U};

    std::vector Layers(std::cbegin(g_RequiredInstanceLayers), std::cend(g_RequiredInstanceLayers));
    std::vector Extensions(std::cbegin(g_RequiredInstanceExtensions), std::cend(g_RequiredInstanceExtensions));

    // ReSharper disable once CppTooWideScopeInitStatement
    std::vector const GLFWExtensions = GetGLFWExtensions();

    for (std::string const &ExtensionIter : GLFWExtensions)
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

    auto const AvailableLayers = GetAvailableInstanceLayersNames();
    GetAvailableResources("instance layer", Layers, g_OptionalInstanceLayers, AvailableLayers);

    auto const AvailableExtensions = GetAvailableInstanceExtensionsNames();
    GetAvailableResources("instance extension", Extensions, g_OptionalInstanceExtensions, AvailableExtensions);

    CreateInfo.enabledLayerCount   = static_cast<std::uint32_t>(std::size(Layers));
    CreateInfo.ppEnabledLayerNames = std::data(Layers);

    CreateInfo.enabledExtensionCount   = static_cast<std::uint32_t>(std::size(Extensions));
    CreateInfo.ppEnabledExtensionNames = std::data(Extensions);

    CheckVulkanResult(vkCreateInstance(&CreateInfo, nullptr, &g_Instance));
    volkLoadInstance(g_Instance);

#ifdef _DEBUG
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Setting up debug messages";
    CheckVulkanResult(CreateDebugUtilsMessenger(g_Instance, &CreateDebugInfo, nullptr, &g_DebugMessenger));
#endif

    return g_Instance != VK_NULL_HANDLE;
}

void Renderer::DrawFrame(GLFWwindow *const Window, float const DeltaTime, Camera const &Camera, Control *const Owner)
{
    RuntimeInfo::Manager::Get().PushCallstack();

    if (!IsInitialized())
    {
        return;
    }

    if (Window == nullptr || Owner == nullptr)
    {
        throw std::runtime_error("Window is invalid");
    }

    m_FrameTime = DeltaTime;

    RemoveInvalidObjects();

    if (HasAnyFlag(m_StateFlags, g_InvalidStatesToRender))
    {
        if (!HasFlag(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION) && HasFlag(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION))
        {
            DestroyCommandsSynchronizationObjects(m_ImageIndex.has_value());
            m_BufferManager.DestroyBufferResources(false);
            m_PipelineManager.ReleaseDynamicPipelineResources();

            RemoveFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
            AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);
        }
        else if (!HasFlag(m_StateFlags, RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE)
                 && HasFlag(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION))
        {
            BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Refreshing resources...";

            auto const SurfaceProperties   = GetSurfaceProperties(Window, m_BufferManager.GetSurface());
            auto const SurfaceCapabilities = GetSurfaceCapabilities(m_BufferManager.GetSurface());

            m_BufferManager.CreateSwapChain(SurfaceProperties, SurfaceCapabilities);

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
            m_BufferManager.CreateViewportResources(SurfaceProperties);
#endif

            m_BufferManager.CreateDepthResources(SurfaceProperties);

            Owner->RefreshResources();
            CreateCommandsSynchronizationObjects();

            RemoveFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);
            AddFlags(m_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }
        else if (HasFlag(m_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH))
        {
            m_PipelineManager.CreateDescriptorSetLayout();
            m_PipelineManager.CreatePipeline(m_BufferManager.GetSwapChainImageFormat(), m_BufferManager.GetDepthFormat(), m_BufferManager.GetSwapChainExtent());
            RemoveFlags(m_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }
    }
    else if (m_ImageIndex = RequestImageIndex(Window); m_ImageIndex.has_value())
    {
#ifdef VULKAN_RENDERER_ENABLE_IMGUI
        DrawImGuiFrame(
            [Owner]
            {
                Owner->PreUpdate();
            },
            [Owner]
            {
                Owner->Update();
            },
            [Owner]
            {
                Owner->PostUpdate();
            });
#endif

        if (!HasAnyFlag(m_StateFlags, g_InvalidStatesToRender) && m_ImageIndex.has_value())
        {
            std::unique_lock Lock(m_RenderingMutex);

            RecordCommandBuffers(m_ImageIndex.value(), Camera, m_BufferManager, m_PipelineManager, m_Objects, m_BufferManager.GetSwapChainExtent());
            SubmitCommandBuffers();
            PresentFrame(m_ImageIndex.value(), m_BufferManager.GetSwapChain());
        }
    }
}

std::optional<std::int32_t> Renderer::RequestImageIndex(GLFWwindow *const Window)
{
    RuntimeInfo::Manager::Get().PushCallstack();

    std::optional<std::int32_t> Output {};

    if (!HasAnyFlag(m_StateFlags, g_InvalidStatesToRender))
    {
        Output = RequestSwapChainImage(m_BufferManager.GetSwapChain());
    }

    if (!GetSurfaceProperties(Window, m_BufferManager.GetSurface()).IsValid())
    {
        AddFlags(m_StateFlags, RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
    }
    else
    {
        RemoveFlags(m_StateFlags, RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
    }

    if (!Output.has_value())
    {
        AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }

    return Output;
}

void Renderer::Tick()
{
    RuntimeInfo::Manager::Get().PushCallstack();

    if (!IsInitialized())
    {
        return;
    }

    static std::uint64_t LastTime    = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::uint64_t const  CurrentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    double const         DeltaTime   = static_cast<double>(CurrentTime - LastTime) / 1000.0;

    if (DeltaTime <= 0.F)
    {
        return;
    }

    m_DeltaTime = DeltaTime;
    LastTime    = CurrentTime;

    std::unique_lock Lock(m_RenderingMutex);

    std::ranges::for_each(m_Objects,
                          [DeltaTime](std::shared_ptr<Object> const &ObjectIter)
                          {
                              if (ObjectIter && !ObjectIter->IsPendingDestroy())
                              {
                                  ObjectIter->Tick(DeltaTime);
                              }
                          });
}

void Renderer::RemoveInvalidObjects()
{
    RuntimeInfo::Manager::Get().PushCallstack();

    if (std::empty(m_Objects))
    {
        return;
    }

    std::unique_lock Lock(m_RenderingMutex);

    std::vector<std::uint32_t> LoadedIDs {};
    for (std::shared_ptr<Object> const &ObjectIter : m_Objects)
    {
        if (ObjectIter && ObjectIter->IsPendingDestroy())
        {
            LoadedIDs.push_back(ObjectIter->GetID());
        }
    }

    if (!std::empty(LoadedIDs))
    {
        UnloadScene(LoadedIDs);
    }
}

bool Renderer::Initialize(GLFWwindow *const Window)
{
    RuntimeInfo::Manager::Get().PushCallstack();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Initializing vulkan renderer";

    if (IsInitialized())
    {
        return false;
    }

    RenderingSubsystem::Get().RegisterRenderer(this);

    CheckVulkanResult(volkInitialize());

    if (!CreateVulkanInstance())
    {
        throw std::runtime_error("Failed to create vulkan instance");
    }

    m_BufferManager.CreateVulkanSurface(Window);
    InitializeDevice(m_BufferManager.GetSurface());
    volkLoadDevice(GetLogicalDevice());

    m_BufferManager.CreateMemoryAllocator(GetPhysicalDevice());
    m_BufferManager.CreateSceneUniformBuffers();
    m_BufferManager.CreateImageSampler();
    auto const _                 = CompileDefaultShaders();
    auto const SurfaceProperties = GetSurfaceProperties(Window, m_BufferManager.GetSurface());

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    InitializeImGuiContext(Window, SurfaceProperties);
#endif

    AddFlags(m_StateFlags, RendererStateFlags::INITIALIZED);
    AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);

    return SurfaceProperties.IsValid() && IsInitialized();
}

void Renderer::Shutdown([[maybe_unused]] GLFWwindow *const Window)
{
    RuntimeInfo::Manager::Get().PushCallstack();

    if (!IsInitialized())
    {
        return;
    }

    RenderingSubsystem::Get().UnregisterRenderer();

    RemoveFlags(m_StateFlags, RendererStateFlags::INITIALIZED);

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    ReleaseImGuiResources();
#endif

    ReleaseShaderResources();
    ReleaseCommandsResources();
    m_BufferManager.ReleaseBufferResources();
    m_PipelineManager.ReleasePipelineResources();
    ReleaseDeviceResources();

    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Shutting down vulkan renderer";

#ifdef _DEBUG
    if (g_DebugMessenger != VK_NULL_HANDLE)
    {
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Shutting down vulkan debug messenger";

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

bool Renderer::IsReady() const
{
    return m_BufferManager.GetSwapChain() != VK_NULL_HANDLE;
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

RendererStateFlags Renderer::GetStateFlags() const
{
    return m_StateFlags;
}

std::vector<std::uint32_t> Renderer::LoadScene(std::string_view const ObjectPath)
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};

    if (!IsInitialized())
    {
        return {};
    }

    if (!std::filesystem::exists(ObjectPath))
    {
        throw std::runtime_error("Object path is invalid");
    }

    std::lock_guard Lock(m_RenderingMutex);

    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Loading scene...";

    std::vector<Object> const  LoadedObjects = m_BufferManager.AllocateScene(ObjectPath);
    std::vector<std::uint32_t> Output;
    Output.reserve(std::size(LoadedObjects));

    for (Object const &ObjectIter : LoadedObjects)
    {
        m_Objects.push_back(std::make_shared<Object>(ObjectIter));
        Output.push_back(ObjectIter.GetID());
    }

    AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);

    return Output;
}

void Renderer::UnloadScene(std::vector<std::uint32_t> const &ObjectIDs)
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};

    if (!IsInitialized())
    {
        return;
    }

    std::unique_lock Lock(m_RenderingMutex);

    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Unloading scene...";

    m_BufferManager.ReleaseScene(ObjectIDs);

    std::ranges::for_each(ObjectIDs,
                          [this](std::uint32_t const ObjectID)
                          {
                              std::erase_if(m_Objects,
                                            [ObjectID](std::shared_ptr<Object> const &ObjectIter)
                                            {
                                                return !ObjectIter || ObjectIter->GetID() == ObjectID;
                                            });
                          });

    AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
}

void Renderer::UnloadAllScenes()
{
    std::vector<std::uint32_t> LoadedIDs {};
    for (std::shared_ptr<Object> const &ObjectIter : m_Objects)
    {
        if (ObjectIter)
        {
            LoadedIDs.push_back(ObjectIter->GetID());
        }
    }
    UnloadScene(LoadedIDs);
    m_Objects.clear();
}

Timer::Manager &Renderer::GetRenderTimerManager()
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

Camera const &Renderer::GetCamera() const
{
    return m_Camera;
}

Camera &Renderer::GetMutableCamera()
{
    return m_Camera;
}

std::optional<std::int32_t> const &Renderer::GetImageIndex() const
{
    return m_ImageIndex;
}

std::vector<std::shared_ptr<Object>> const &Renderer::GetObjects() const
{
    return m_Objects;
}

std::shared_ptr<Object> Renderer::GetObjectByID(std::uint32_t const ObjectID) const
{
    return *std::ranges::find_if(m_Objects,
                                 [ObjectID](std::shared_ptr<Object> const &ObjectIter)
                                 {
                                     return ObjectIter && ObjectIter->GetID() == ObjectID;
                                 });
}

std::uint32_t Renderer::GetNumObjects() const
{
    return std::size(m_Objects);
}

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
std::vector<VkImageView> Renderer::GetViewportRenderImageViews() const
{
    std::vector<VkImageView> Output;
    auto const              &ViewportAllocations = m_BufferManager.GetViewportImages();
    Output.reserve(std::size(ViewportAllocations));

    for (auto const &[Image, View, Allocation, Type] : ViewportAllocations)
    {
        Output.push_back(View);
    }

    return Output;
}
#endif

VkSampler Renderer::GetSampler() const
{
    return m_BufferManager.GetSampler();
}

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
bool Renderer::IsImGuiInitialized()
{
    return RenderCore::IsImGuiInitialized();
}

void Renderer::SaveFrameToImageFile(std::string_view const Path) const
{
    RuntimeInfo::Manager::Get().PushCallstack();
    m_BufferManager.SaveImageToFile(m_BufferManager.GetViewportImages().at(GetImageIndex().value()).Image, Path);
}
#endif
