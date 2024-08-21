// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#ifndef VOLK_IMPLEMENTATION
#define VOLK_IMPLEMENTATION
#endif
#include <Volk/volk.h>

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
import RenderCore.Integrations.Offscreen;
import RenderCore.Integrations.ImGuiOverlay;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumHelpers;
import RenderCore.Utils.Constants;
import RenderCore.Types.Allocation;

using namespace RenderCore;

auto                       g_StateFlags { RendererStateFlags::NONE };
auto                       g_ObjectsManagementStateFlags { RendererObjectsManagementStateFlags::NONE };
std::vector<strzilla::string>   g_ModelsToLoad {};
std::vector<std::uint32_t> g_ModelsToUnload {};
double                     g_FrameTime { 0.F };
double                     g_FrameRateCap { 0.016667F };
bool                       g_UseVSync { true };
bool                       g_RenderOffscreen { false };
InitializationFlags        g_InitializationFlags { InitializationFlags::NONE };
std::uint32_t              g_ImageIndex { g_ImageCount };

constexpr RendererStateFlags g_InvalidStatesToRender = RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE |
                                                       RendererStateFlags::PENDING_RESOURCES_DESTRUCTION |
                                                       RendererStateFlags::PENDING_RESOURCES_CREATION | RendererStateFlags::PENDING_PIPELINE_REFRESH |
                                                       RendererStateFlags::INVALID_SIZE;

void RenderCore::DrawFrame(GLFWwindow *const Window, double const DeltaTime, Control *const Owner)
{
    g_FrameTime = DeltaTime;

    if (HasAnyFlag(g_ObjectsManagementStateFlags))
    {
        AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }

    if (HasAnyFlag(g_StateFlags, g_InvalidStatesToRender))
    {
        if (!HasFlag(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION) && HasFlag(g_StateFlags,
                                                                                              RendererStateFlags::PENDING_RESOURCES_DESTRUCTION))
        {
            CheckVulkanResult(vkDeviceWaitIdle(GetLogicalDevice()));

            g_ImageIndex = g_ImageCount;

            for (std::uint8_t Iterator = 0U; Iterator < g_ImageCount; ++Iterator)
            {
                ResetCommandPool(Iterator);
            }

            ResetFenceStatus();
            DestroySwapChainImages();
            DestroyOffscreenImages();
            ReleasePipelineResources(false);

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
                RemoveFlags(g_ObjectsManagementStateFlags,
                            RendererObjectsManagementStateFlags::PENDING_CLEAR | RendererObjectsManagementStateFlags::PENDING_UNLOAD);
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

            RemoveFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
            AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);
        }
        else if (!HasAnyFlag(g_StateFlags, RendererStateFlags::INVALID_SIZE | RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE) &&
                 HasFlag(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION))
        {
            auto const SurfaceProperties = GetSurfaceProperties(Window);

            if (!SurfaceProperties.IsValid())
            {
                AddFlags(g_StateFlags, RendererStateFlags::INVALID_SIZE);
                return;
            }

            auto const SurfaceCapabilities = GetSurfaceCapabilities();

            CreateSwapChain(SurfaceProperties, SurfaceCapabilities);
            CreateDepthResources(SurfaceProperties);

            if (!HasFlag(g_StateFlags, RendererStateFlags::INITIALIZED))
            {
                SetupPipelineLayouts();
                CreatePipelineLibraries();

                if (HasFlag(g_InitializationFlags, InitializationFlags::ENABLE_IMGUI))
                {
                    InitializeImGuiContext(Window,
                        HasFlag(g_InitializationFlags, InitializationFlags::ENABLE_DOCKING),
                        HasFlag(g_InitializationFlags, InitializationFlags::ENABLE_VIEWPORTS));
                }

                Owner->Initialize();

                AddFlags(g_StateFlags, RendererStateFlags::INITIALIZED);
            }

            if (g_RenderOffscreen)
            {
                CreateOffscreenResources(SurfaceProperties);
            }

            Owner->RefreshResources();

            RemoveFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION | RendererStateFlags::INVALID_SIZE);
            AddFlags(g_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }
        else if (HasFlag(g_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH))
        {
            CreatePipelineDynamicResources();
            PipelineDescriptorData &PipelineDescriptor = GetPipelineDescriptorData();
            PipelineDescriptor.SetupSceneBuffer(GetSceneUniformBuffer());
            PipelineDescriptor.SetupModelsBuffer(GetObjects());
            SetNumObjectsPerThread(GetNumAllocations());

            RemoveFlags(g_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }
    }
    else if (RequestSwapChainImage(g_ImageIndex))
    {
        DrawImGuiFrame(Owner);

        UpdateSceneUniformBuffer();
        Tick();

        RecordCommandBuffers(g_ImageIndex);
        SubmitCommandBuffers(g_ImageIndex);
        PresentFrame(g_ImageIndex);
    }
}

void RenderCore::Tick()
{
    GetCamera().UpdateCameraMovement(g_FrameTime);
    TickObjects(static_cast<float>(g_FrameTime));
}

bool RenderCore::Initialize(GLFWwindow *const Window, InitializationFlags const Flags)
{
    if (Renderer::IsInitialized())
    {
        return false;
    }

    g_InitializationFlags = Flags;

    CheckVulkanResult(volkInitialize());

    [[maybe_unused]] bool const _ = CreateVulkanInstance();
    CreateVulkanSurface(Window);
    InitializeDevice(GetSurface());
    volkLoadDevice(GetLogicalDevice());

    InitializeCommandsResources(GetGraphicsQueue().first);
    CreateSynchronizationObjects();
    CreateMemoryAllocator();
    CreateSceneUniformBuffer();
    CreateImageSampler();
    CompileDefaultShaders();
    auto const SurfaceProperties = GetSurfaceProperties(Window);
    AllocateEmptyTexture(SurfaceProperties.Format.format);

    AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION);

    return SurfaceProperties.IsValid();
}

void RenderCore::Shutdown(Control *Window)
{
    if (!Renderer::IsInitialized())
    {
        return;
    }

    ReleaseSynchronizationObjects();
    ReleaseCommandsResources();

    if (Window)
    {
        Window->DestroyChildren(true);
    }

    if (HasFlag(g_InitializationFlags, InitializationFlags::ENABLE_IMGUI))
    {
        ReleaseImGuiResources();
    }

    DestroyOffscreenImages();

    ReleaseSwapChainResources();
    ReleaseShaderResources();
    ReleaseSceneResources();
    ReleasePipelineResources(true);
    ReleaseMemoryResources();
    ReleaseDeviceResources();
    DestroyVulkanInstance();

    g_StateFlags = RendererStateFlags::NONE;
}

bool Renderer::IsInitialized()
{
    return HasAnyFlag(g_StateFlags, RendererStateFlags::INITIALIZED | RendererStateFlags::PENDING_RESOURCES_CREATION);
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

void Renderer::RequestLoadObject(strzilla::string_view const ObjectPath)
{
    g_ModelsToLoad.emplace_back(ObjectPath);
    AddFlags(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_LOAD);
}

void Renderer::RequestUnloadObjects(std::vector<std::uint32_t> const &ObjectIDs)
{
    g_ModelsToUnload.insert(std::end(g_ModelsToUnload), std::begin(ObjectIDs), std::end(ObjectIDs));
    AddFlags(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_UNLOAD);
}

void Renderer::RequestClearScene()
{
    AddFlags(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_CLEAR);
}

void Renderer::RequestUpdateResources()
{
    AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
}

double const &Renderer::GetFrameTime()
{
    return g_FrameTime;
}

void Renderer::SetFPSLimit(double const MaxFPS)
{
    if (MaxFPS > 0.0)
    {
        g_FrameRateCap = 1.0 / MaxFPS;
    }
}

double const &Renderer::GetFPSLimit()
{
    return g_FrameRateCap;
}

bool const &Renderer::GetVSync()
{
    return g_UseVSync;
}

void Renderer::SetVSync(bool const Value)
{
    g_UseVSync = Value;
}

bool const &Renderer::GetRenderOffscreen()
{
    return g_RenderOffscreen;
}

void Renderer::SetRenderOffscreen(bool const Value)
{
    if (g_RenderOffscreen != Value)
    {
        g_RenderOffscreen = Value;
    }
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

std::uint32_t const &Renderer::GetImageIndex()
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

std::vector<VkImageView> Renderer::GetOffscreenImages()
{
    std::vector<VkImageView> Output;
    auto const &             OffscreenAllocations = RenderCore::GetOffscreenImages();
    Output.reserve(std::size(OffscreenAllocations));

    for (ImageAllocation const &AllocationIter : OffscreenAllocations)
    {
        Output.push_back(AllocationIter.View);
    }

    return Output;
}

bool Renderer::IsImGuiInitialized()
{
    return RenderCore::IsImGuiInitialized();
}

void Renderer::SaveOffscreenFrameToImage(strzilla::string_view const Path)
{
    ImageAllocation const &OffscreenImage = RenderCore::GetOffscreenImages().at(g_ImageIndex);
    SaveImageToFile(OffscreenImage.Image, Path, OffscreenImage.Extent);
}

void Renderer::PrintMemoryAllocatorStats(bool const DetailedMap)
{
    RenderCore::PrintMemoryAllocatorStats(DetailedMap);
}

strzilla::string Renderer::GetMemoryAllocatorStats(bool const DetailedMap)
{
    return RenderCore::GetMemoryAllocatorStats(DetailedMap);
}

InitializationFlags Renderer::GetWindowInitializationFlags()
{
    return g_InitializationFlags;
}