// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

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
#include <execution>
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
import RenderCore.Runtime.Instance;
import RenderCore.Integrations.Viewport;
import RenderCore.Integrations.ImGuiOverlay;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumHelpers;
import RenderCore.Utils.Constants;
import RenderCore.Types.Allocation;

using namespace RenderCore;

RendererStateFlags          g_StateFlags { RendererStateFlags::NONE };
double                      g_FrameTime { 0.F };
double                      g_FrameRateCap { 0.016667F };
std::optional<std::int32_t> g_ImageIndex {};
std::mutex                  g_RenderingMutex {};

constexpr RendererStateFlags g_InvalidStatesToRender = RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE |
                                                       RendererStateFlags::PENDING_RESOURCES_DESTRUCTION |
                                                       RendererStateFlags::PENDING_RESOURCES_CREATION | RendererStateFlags::PENDING_PIPELINE_REFRESH;

void RenderCore::DrawFrame(GLFWwindow *const Window, double const DeltaTime, Control *const Owner)
{
    if (!Renderer::IsInitialized())
    {
        return;
    }

    g_FrameTime = DeltaTime;

    if (HasAnyFlag(g_StateFlags, g_InvalidStatesToRender))
    {
        if (!HasFlag(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION) && HasFlag(g_StateFlags,
                                                                                              RendererStateFlags::PENDING_RESOURCES_DESTRUCTION))
        {
            ReleaseSynchronizationObjects();
            DestroySwapChainImages();
            #ifdef VULKAN_RENDERER_ENABLE_IMGUI
            DestroyViewportImages();
            #endif
            ReleaseDynamicPipelineResources();

            RemoveFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
            AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);
        }
        else if (!HasFlag(g_StateFlags, RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE) && HasFlag(g_StateFlags,
                     RendererStateFlags::PENDING_RESOURCES_CREATION))
        {
            auto const SurfaceProperties   = GetSurfaceProperties(Window);
            auto const SurfaceCapabilities = GetSurfaceCapabilities();

            CreateSwapChain(SurfaceProperties, SurfaceCapabilities);

            #ifdef VULKAN_RENDERER_ENABLE_IMGUI
            CreateViewportResources(SurfaceProperties);
            #endif

            CreateDepthResources(SurfaceProperties);

            Owner->RefreshResources();
            CreateSynchronizationObjects();

            RemoveFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);
            AddFlags(g_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }
        else if (HasFlag(g_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH))
        {
            CreateDescriptorSetLayout();
            CreatePipeline();
            RemoveFlags(g_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }
    }
    else if (g_ImageIndex = RequestImageIndex(Window);
        g_ImageIndex.has_value())
    {
        Tick();

        #ifdef VULKAN_RENDERER_ENABLE_IMGUI
        DrawImGuiFrame([Owner]
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

        if (!HasAnyFlag(g_StateFlags, g_InvalidStatesToRender) && g_ImageIndex.has_value())
        {
            std::lock_guard Lock(g_RenderingMutex);

            UpdateSceneUniformBuffers();
            RecordCommandBuffers(g_ImageIndex.value());
            SubmitCommandBuffers();
            PresentFrame(g_ImageIndex.value());
        }
    }
}

std::optional<std::int32_t> RenderCore::RequestImageIndex(GLFWwindow *const Window)
{
    std::optional<std::int32_t> Output {};

    if (!HasAnyFlag(g_StateFlags, g_InvalidStatesToRender))
    {
        Output = RequestSwapChainImage();
    }

    if (!GetSurfaceProperties(Window).IsValid())
    {
        AddFlags(g_StateFlags, RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
    }
    else
    {
        RemoveFlags(g_StateFlags, RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
    }

    if (!Output.has_value())
    {
        AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }

    return Output;
}

void RenderCore::Tick()
{
    GetCamera().UpdateCameraMovement(g_FrameTime);

    auto const &Objects = GetObjects();

    std::for_each(std::execution::par_unseq,
                  std::begin(Objects),
                  std::end(Objects),
                  [](std::shared_ptr<Object> const &ObjectIter)
                  {
                      if (ObjectIter && !ObjectIter->IsPendingDestroy())
                      {
                          ObjectIter->Tick(g_FrameTime);
                      }
                  });
}

bool RenderCore::Initialize(GLFWwindow *const Window)
{
    if (Renderer::IsInitialized())
    {
        return false;
    }

    CheckVulkanResult(volkInitialize());

    [[maybe_unused]] bool const _1 = CreateVulkanInstance();
    CreateVulkanSurface(Window);
    InitializeDevice(GetSurface());
    volkLoadDevice(GetLogicalDevice());

    CreateMemoryAllocator(GetPhysicalDevice());
    CreateSceneUniformBuffers();
    CreateImageSampler();
    [[maybe_unused]] auto const _2                = CompileDefaultShaders();
    auto const                  SurfaceProperties = GetSurfaceProperties(Window);
    AllocateEmptyTexture(SurfaceProperties.Format.format);

    #ifdef VULKAN_RENDERER_ENABLE_IMGUI
    InitializeImGuiContext(Window, SurfaceProperties);
    #endif

    AddFlags(g_StateFlags, RendererStateFlags::INITIALIZED);
    AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);

    return SurfaceProperties.IsValid() && Renderer::IsInitialized();
}

void RenderCore::Shutdown([[maybe_unused]] GLFWwindow *const Window)
{
    if (!Renderer::IsInitialized())
    {
        return;
    }

    RemoveFlags(g_StateFlags, RendererStateFlags::INITIALIZED);

    #ifdef VULKAN_RENDERER_ENABLE_IMGUI
    ReleaseImGuiResources();
    DestroyViewportImages();
    #endif

    ReleaseSwapChainResources();
    ReleaseShaderResources();
    ReleaseCommandsResources();
    ReleaseSynchronizationObjects();
    ReleaseSceneResources();
    ReleaseMemoryResources();
    ReleasePipelineResources();
    ReleaseDeviceResources();
    DestroyVulkanInstance();
}

bool Renderer::IsInitialized()
{
    return HasFlag(g_StateFlags, RendererStateFlags::INITIALIZED);
}

bool Renderer::IsReady()
{
    return GetSwapChain() != VK_NULL_HANDLE;
}

void Renderer::AddStateFlag(RendererStateFlags const Flag)
{
    AddFlags(g_StateFlags, Flag);
}

void Renderer::RemoveStateFlag(RendererStateFlags const Flag)
{
    RemoveFlags(g_StateFlags, Flag);
}

bool Renderer::HasStateFlag(RendererStateFlags const Flag)
{
    return HasFlag(g_StateFlags, Flag);
}

RendererStateFlags Renderer::GetStateFlags()
{
    return g_StateFlags;
}

void Renderer::LoadObject(std::string_view const ObjectPath)
{
    if (!IsInitialized())
    {
        return;
    }

    std::lock_guard Lock(g_RenderingMutex);

    RenderCore::LoadObject(ObjectPath);
    AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
}

void Renderer::UnloadObjects(std::vector<std::uint32_t> const &ObjectIDs)
{
    if (!IsInitialized())
    {
        return;
    }

    std::lock_guard Lock(g_RenderingMutex);

    RenderCore::UnloadObjects(ObjectIDs);
    AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
}

void Renderer::DestroyObjects()
{
    RenderCore::DestroyObjects();
}

double Renderer::GetFrameTime()
{
    return g_FrameTime;
}

void Renderer::SetFrameRateCap(double const FrameRateCap)
{
    if (FrameRateCap > 0.0)
    {
        g_FrameRateCap = 1.0 / FrameRateCap;
    }
}

double Renderer::GetFrameRateCap()
{
    return g_FrameRateCap;
}

Camera const &Renderer::GetCamera()
{
    return RenderCore::GetCamera();
}

Camera &Renderer::GetMutableCamera()
{
    return RenderCore::GetCamera();
}

Illumination const &Renderer::GetIllumination()
{
    return RenderCore::GetIllumination();
}

Illumination &Renderer::GetMutableIllumination()
{
    return RenderCore::GetIllumination();
}

std::optional<std::int32_t> const &Renderer::GetImageIndex()
{
    return g_ImageIndex;
}

std::vector<std::shared_ptr<Object>> const &Renderer::GetObjects()
{
    return RenderCore::GetObjects();
}

std::shared_ptr<Object> Renderer::GetObjectByID(std::uint32_t const ObjectID)
{
    return *std::ranges::find_if(RenderCore::GetObjects(),
                                 [ObjectID](std::shared_ptr<Object> const &ObjectIter)
                                 {
                                     return ObjectIter && ObjectIter->GetID() == ObjectID;
                                 });
}

std::uint32_t Renderer::GetNumObjects()
{
    return std::size(RenderCore::GetObjects());
}

VkSampler Renderer::GetSampler()
{
    return RenderCore::GetSampler();
}

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
std::vector<VkImageView> Renderer::GetViewportImages()
{
    std::vector<VkImageView> Output;
    auto const &             ViewportAllocations = RenderCore::GetViewportImages();
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

void Renderer::SaveFrameAsImage(std::string_view const Path)
{
    SaveImageToFile(RenderCore::GetViewportImages().at(GetImageIndex().value()).Image, Path, GetSwapChainExtent());
}
#endif
