// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <filesystem>
#include <vector>

// Include vulkan before glfw
#ifndef VOLK_IMPLEMENTATION
#define VOLK_IMPLEMENTATION
#endif
#include <Volk/volk.h>

// Include glfw after vulkan
#include <execution>
#include <future>
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

auto                        g_StateFlags { RendererStateFlags::NONE };
auto                        g_ObjectsManagementStateFlags { RendererObjectsManagementStateFlags::NONE };
std::vector<std::string>    g_ModelsToLoad {};
std::vector<std::uint32_t>  g_ModelsToUnload {};
double                      g_FrameTime { 0.F };
double                      g_FrameRateCap { 0.016667F };
bool                        g_UseVSync { true };
std::optional<std::int32_t> g_ImageIndex {};

constexpr RendererStateFlags g_InvalidStatesToRender = RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE |
                                                       RendererStateFlags::PENDING_RESOURCES_DESTRUCTION |
                                                       RendererStateFlags::PENDING_RESOURCES_CREATION | RendererStateFlags::PENDING_PIPELINE_REFRESH;

void RenderCore::DrawFrame(GLFWwindow *const Window, double const DeltaTime, Control *const Owner)
{
    g_FrameTime = DeltaTime;
    CheckObjectManagementFlags();

    if (HasAnyFlag(g_StateFlags, g_InvalidStatesToRender))
    {
        if (!HasFlag(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION) && HasFlag(g_StateFlags,
                                                                                              RendererStateFlags::PENDING_RESOURCES_DESTRUCTION))
        {
            ReleaseCommandsResources();
            ReleaseSynchronizationObjects();
            DestroySwapChainImages();
            #ifdef VULKAN_RENDERER_ENABLE_IMGUI
            DestroyViewportImages();
            #endif
            ReleasePipelineResources();

            RemoveFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
            AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);
        }
        else if (!HasFlag(g_StateFlags, RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE) && HasFlag(g_StateFlags,
                     RendererStateFlags::PENDING_RESOURCES_CREATION))
        {
            auto const SurfaceProperties   = GetSurfaceProperties(Window);
            auto const SurfaceCapabilities = GetSurfaceCapabilities();

            InitializeCommandsResources();
            CreateSwapChain(SurfaceProperties, SurfaceCapabilities);

            #ifdef VULKAN_RENDERER_ENABLE_IMGUI
            CreateViewportResources(SurfaceProperties);
            #endif

            CreateDepthResources(SurfaceProperties);

            Owner->RefreshResources();
            CreateSynchronizationObjects();
            AllocateCommandBuffers(GetGraphicsQueue().first, GetNumAllocations());

            RemoveFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);
            AddFlags(g_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }
        else if (HasFlag(g_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH))
        {
            SetupPipelineLayouts();
            CreatePipelineLibraries();
            CreatePipeline();
            PipelineDescriptorData &PipelineDescriptor = GetPipelineDescriptorData();
            PipelineDescriptor.SetupSceneBuffer(GetSceneUniformBuffer());
            PipelineDescriptor.SetupModelsBuffer(GetObjects());

            RemoveFlags(g_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }
    }
    else if (g_ImageIndex = RequestImageIndex();
        g_ImageIndex.has_value())
    {
        Tick();

        #ifdef VULKAN_RENDERER_ENABLE_IMGUI
        DrawImGuiFrame(Owner);
        #endif

        if (!HasAnyFlag(g_StateFlags, g_InvalidStatesToRender) && g_ImageIndex.has_value())
        {
            std::int32_t const ImageIndex = g_ImageIndex.value();

            UpdateSceneUniformBuffer();
            RecordCommandBuffers(ImageIndex);
            SubmitCommandBuffers();
            PresentFrame(ImageIndex);
        }
    }
}

std::optional<std::int32_t> RenderCore::RequestImageIndex()
{
    std::optional<std::int32_t> Output {};

    if (!HasAnyFlag(g_StateFlags, g_InvalidStatesToRender))
    {
        Output = RequestSwapChainImage();
    }

    if (!Output.has_value())
    {
        AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }

    return Output;
}

void RenderCore::CheckObjectManagementFlags()
{
    if (HasAnyFlag(g_ObjectsManagementStateFlags))
    {
        AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }

    if (HasAnyFlag(g_ObjectsManagementStateFlags,
                   RendererObjectsManagementStateFlags::PENDING_CLEAR | RendererObjectsManagementStateFlags::PENDING_UNLOAD))
    {
        if (HasFlag(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_CLEAR))
        {
            DestroyObjects();
        }
        else if (HasFlag(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_UNLOAD))
        {
            UnloadObjects(g_ModelsToUnload);
        }

        g_ModelsToUnload.clear();
        RemoveFlags(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_CLEAR);
        RemoveFlags(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_UNLOAD);
    }

    if (HasFlag(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_LOAD))
    {
        for (auto const &ModelPath : g_ModelsToLoad)
        {
            LoadScene(ModelPath);
        }

        g_ModelsToLoad.clear();
        RemoveFlags(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_LOAD);
    }
}

void RenderCore::Tick()
{
    GetCamera().UpdateCameraMovement(g_FrameTime);
    TickObjects(static_cast<float>(g_FrameTime));
}

bool RenderCore::Initialize(GLFWwindow *const Window)
{
    if (Renderer::IsInitialized())
    {
        return false;
    }

    CheckVulkanResult(volkInitialize());

    [[maybe_unused]] bool const _ = CreateVulkanInstance();
    CreateVulkanSurface(Window);
    InitializeDevice(GetSurface());
    volkLoadDevice(GetLogicalDevice());

    InitializeCommandsResources();
    CreateMemoryAllocator();
    CreateSceneUniformBuffer();
    CreateImageSampler();
    CompileDefaultShaders();
    auto const SurfaceProperties = GetSurfaceProperties(Window);
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
    ReleasePipelineResources();
    ReleaseMemoryResources();
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

void Renderer::RequestLoadObject(std::string_view const ObjectPath)
{
    g_ModelsToLoad.emplace_back(ObjectPath);
    AddFlags(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_LOAD);
}

void Renderer::RequestUnloadObjects(std::vector<std::uint32_t> const &ObjectIDs)
{
    g_ModelsToUnload.insert(std::end(g_ModelsToUnload), std::begin(ObjectIDs), std::end(ObjectIDs));
    AddFlags(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_UNLOAD);
}

void Renderer::RequestDestroyObjects()
{
    AddFlags(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_CLEAR);
}

void Renderer::RequestUpdateResources()
{
    AddStateFlag(RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
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

bool Renderer::GetUseVSync()
{
    return g_UseVSync;
}

void Renderer::SetUseVSync(bool const Value)
{
    g_UseVSync = Value;
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

std::vector<std::shared_ptr<Object>> &Renderer::GetMutableObjects()
{
    return RenderCore::GetObjects();
}

std::shared_ptr<Object> Renderer::GetObjectByID(std::uint32_t const ObjectID)
{
    return *std::ranges::find_if(RenderCore::GetObjects(),
                                 [ObjectID](std::shared_ptr<Object> const &ObjectIter)
                                 {
                                     return ObjectIter->GetID() == ObjectID;
                                 });
}

std::uint32_t Renderer::GetNumObjects()
{
    return static_cast<std::uint32_t>(std::size(RenderCore::GetObjects()));
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
    ImageAllocation const &ViewportImage = RenderCore::GetViewportImages().at(GetImageIndex().value());
    SaveImageToFile(ViewportImage.Image, Path, ViewportImage.Extent);
}
#endif
