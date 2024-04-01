// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <filesystem>
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

import RenderCore.Runtime.Device;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.ShaderCompiler;
import RenderCore.Runtime.Command;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Scene;
import RenderCore.Runtime.Model;
import RenderCore.Runtime.SwapChain;
import RenderCore.Runtime.Synchronization;
import RenderCore.Integrations.Viewport;
import RenderCore.Integrations.ImGuiOverlay;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumHelpers;
import RenderCore.Utils.DebugHelpers;
import RenderCore.Utils.Constants;
import RenderCore.Types.Allocation;
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
    CheckVulkanResult(CreateDebugUtilsMessenger(g_Instance, &CreateDebugInfo, nullptr, &g_DebugMessenger));
#endif

    return g_Instance != VK_NULL_HANDLE;
}

void Renderer::DrawFrame(GLFWwindow *const Window, float const DeltaTime, Camera const &Camera, Control *const Owner)
{
    if (!IsInitialized())
    {
        return;
    }

    m_FrameTime = DeltaTime;

    RemoveInvalidObjects();

    if (HasAnyFlag(m_StateFlags, g_InvalidStatesToRender))
    {
        if (!HasFlag(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION) && HasFlag(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION))
        {
            ReleaseSynchronizationObjects();
            DestroySwapChainImages();
            DestroyViewportImages();
            ReleaseDynamicPipelineResources();

            RemoveFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
            AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);
        }
        else if (!HasFlag(m_StateFlags, RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE)
                 && HasFlag(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION))
        {
            auto const SurfaceProperties   = GetSurfaceProperties(Window, GetSurface());
            auto const SurfaceCapabilities = GetSurfaceCapabilities(GetSurface());

            CreateSwapChain(SurfaceProperties, SurfaceCapabilities);

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
            CreateViewportResources(SurfaceProperties);
#endif

            CreateDepthResources(SurfaceProperties);

            Owner->RefreshResources();
            CreateSynchronizationObjects();

            RemoveFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);
            AddFlags(m_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }
        else if (HasFlag(m_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH))
        {
            CreateDescriptorSetLayout();
            CreatePipeline(GetSwapChainImageFormat(), GetDepthImage().Format, GetSwapChainExtent());
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

            UpdateSceneUniformBuffers(Camera, m_Illumination);
            RecordCommandBuffers(m_ImageIndex.value(), Camera, GetSwapChainExtent());
            SubmitCommandBuffers();
            PresentFrame(m_ImageIndex.value());
        }
    }
}

std::optional<std::int32_t> Renderer::RequestImageIndex(GLFWwindow *const Window)
{
    std::optional<std::int32_t> Output {};

    if (!HasAnyFlag(m_StateFlags, g_InvalidStatesToRender))
    {
        Output = RequestSwapChainImage();
    }

    if (!GetSurfaceProperties(Window, GetSurface()).IsValid())
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

    std::ranges::for_each(GetAllocatedObjects(),
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
    std::unique_lock Lock(m_RenderingMutex);

    std::vector<std::uint32_t> LoadedIDs {};
    for (std::shared_ptr<Object> const &ObjectIter : GetAllocatedObjects())
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
    if (IsInitialized())
    {
        return false;
    }

    RenderingSubsystem::Get().RegisterRenderer(this);

    CheckVulkanResult(volkInitialize());

    CreateVulkanInstance();
    CreateVulkanSurface(Window);
    InitializeDevice(GetSurface());
    volkLoadDevice(GetLogicalDevice());

    CreateMemoryAllocator(GetPhysicalDevice());
    CreateSceneUniformBuffers();
    CreateImageSampler();
    auto const _                 = CompileDefaultShaders();
    auto const SurfaceProperties = GetSurfaceProperties(Window, GetSurface());
    AllocateEmptyTexture(SurfaceProperties.Format.format);

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    InitializeImGuiContext(Window, SurfaceProperties);
#endif

    AddFlags(m_StateFlags, RendererStateFlags::INITIALIZED);
    AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);

    return SurfaceProperties.IsValid() && IsInitialized();
}

void Renderer::Shutdown([[maybe_unused]] GLFWwindow *const Window)
{
    if (!IsInitialized())
    {
        return;
    }

    RenderingSubsystem::Get().UnregisterRenderer();

    RemoveFlags(m_StateFlags, RendererStateFlags::INITIALIZED);

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    ReleaseImGuiResources();
#endif

    ReleaseSwapChainResources();
    DestroyViewportImages();
    ReleaseShaderResources();
    ReleaseCommandsResources();
    ReleaseSynchronizationObjects();
    ReleaseSceneResources();
    ReleaseMemoryResources();
    ReleasePipelineResources();
    ReleaseDeviceResources();

#ifdef _DEBUG
    if (g_DebugMessenger != VK_NULL_HANDLE)
    {
        DestroyDebugUtilsMessenger(g_Instance, g_DebugMessenger, nullptr);
        g_DebugMessenger = VK_NULL_HANDLE;
    }
#endif

    vkDestroyInstance(g_Instance, nullptr);
    g_Instance = VK_NULL_HANDLE;
}

bool Renderer::IsInitialized() const
{
    return HasFlag(m_StateFlags, RendererStateFlags::INITIALIZED);
}

bool Renderer::IsReady() const
{
    return GetSwapChain() != VK_NULL_HANDLE;
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

void Renderer::LoadScene(std::string_view const ObjectPath)
{
    if (!IsInitialized())
    {
        return;
    }

    std::lock_guard Lock(m_RenderingMutex);

    AllocateScene(ObjectPath);
    AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
}

void Renderer::UnloadScene(std::vector<std::uint32_t> const &ObjectIDs)
{
    if (!IsInitialized())
    {
        return;
    }

    std::unique_lock Lock(m_RenderingMutex);

    ReleaseScene(ObjectIDs);
    AddFlags(m_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
}

void Renderer::UnloadAllScenes()
{
    DestroyObjects();
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

Illumination const &Renderer::GetIllumination() const
{
    return m_Illumination;
}

Illumination &Renderer::GetMutableIllumination()
{
    return m_Illumination;
}

std::optional<std::int32_t> const &Renderer::GetImageIndex() const
{
    return m_ImageIndex;
}

std::vector<std::shared_ptr<Object>> const &Renderer::GetObjects() const
{
    return GetAllocatedObjects();
}

std::shared_ptr<Object> Renderer::GetObjectByID(std::uint32_t const ObjectID) const
{
    return *std::ranges::find_if(GetAllocatedObjects(),
                                 [ObjectID](std::shared_ptr<Object> const &ObjectIter)
                                 {
                                     return ObjectIter && ObjectIter->GetID() == ObjectID;
                                 });
}

std::uint32_t Renderer::GetNumObjects() const
{
    return std::size(GetAllocatedObjects());
}

VkSampler Renderer::GetSampler() const
{
    return RenderCore::GetSampler();
}

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
std::vector<VkImageView> Renderer::GetViewportRenderImageViews() const
{
    std::vector<VkImageView> Output;
    auto const              &ViewportAllocations = GetViewportImages();
    Output.reserve(std::size(ViewportAllocations));

    for (ImageAllocation const &AllocationIter : ViewportAllocations)
    {
        Output.push_back(AllocationIter.View);
    }

    return Output;
}

bool Renderer::IsImGuiInitialized()
{
    return RenderCore::IsImGuiInitialized();
}

void Renderer::SaveFrameToImageFile(std::string_view const Path) const
{
    SaveImageToFile(GetViewportImages().at(GetImageIndex().value()).Image, Path, GetSwapChainExtent());
}
#endif
