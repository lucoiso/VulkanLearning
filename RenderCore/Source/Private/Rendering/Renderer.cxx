// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Renderer;

import RenderCore.Runtime.Command;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Instance;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Model;
import RenderCore.Runtime.Offscreen;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.Scene;
import RenderCore.Runtime.ShaderCompiler;
import RenderCore.Runtime.SwapChain;
import RenderCore.Runtime.Synchronization;
import RenderCore.Factories.Texture;
import RenderCore.Utils.Helpers;

using namespace RenderCore;

void Renderer::DrawFrame(GLFWwindow *const Window, double const DeltaTime)
{
    std::lock_guard const Lock { g_RendererMutex };

    g_FrameTime = static_cast<float>(DeltaTime);

    DispatchQueue(g_NextTickDispatchQueue);

    if (HasAnyFlag(g_ObjectsManagementStateFlags))
    {
        AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }

    constexpr RendererStateFlags InvalidStatesToRender = RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE |
                                                         RendererStateFlags::PENDING_RESOURCES_DESTRUCTION |
                                                         RendererStateFlags::PENDING_RESOURCES_CREATION |
                                                         RendererStateFlags::PENDING_PIPELINE_REFRESH |
                                                         RendererStateFlags::INVALID_SIZE;

    if (HasAnyFlag(g_StateFlags, InvalidStatesToRender))
    {
        if (HasFlag(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION))
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

        if (!HasAnyFlag(g_StateFlags, RendererStateFlags::INVALID_SIZE | RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE) &&
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

                g_OnInitializeCallback();

                AddFlags(g_StateFlags, RendererStateFlags::INITIALIZED);
            }

            if (g_RenderOffscreen)
            {
                CreateOffscreenResources(SurfaceProperties);
            }

            g_OnRefreshCallback();

            RemoveFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_CREATION | RendererStateFlags::INVALID_SIZE);
            AddFlags(g_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }

        if (HasFlag(g_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH))
        {
            CreatePipelineDynamicResources();
            PipelineDescriptorData &PipelineDescriptor = GetPipelineDescriptorData();
            PipelineDescriptor.SetupSceneBuffer(GetSceneUniformBuffer());
            PipelineDescriptor.SetupModelsBuffer(GetObjects());
            SetNumObjectsPerThread(GetNumAllocations());

            RemoveFlags(g_StateFlags, RendererStateFlags::PENDING_PIPELINE_REFRESH);
        }
    }

    if (!HasAnyFlag(g_StateFlags, InvalidStatesToRender) && RequestSwapChainImage(g_ImageIndex))
    {
        g_OnDrawCallback();

        UpdateSceneUniformBuffer();
        Tick();

        RecordCommandBuffers(g_ImageIndex);
        SubmitCommandBuffers(g_ImageIndex);
        PresentFrame(g_ImageIndex);
    }
}

void Renderer::Tick()
{
    RenderCore::GetCamera().UpdateCameraMovement(g_FrameTime);
    TickObjects(g_FrameTime);
}

bool Renderer::Initialize(GLFWwindow *const Window)
{
    if (IsInitialized())
    {
        return false;
    }

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

void Renderer::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    std::lock_guard const Lock { g_RendererMutex };

    ReleaseSynchronizationObjects();
    ReleaseCommandsResources();

    g_OnShutdownCallback();

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

void Renderer::SetVSync(bool const Value)
{
    DispatchToNextTick([Value]
    {
        g_UseVSync = Value;
    });

    RequestUpdateResources();
}

void Renderer::SetRenderOffscreen(bool const Value)
{
    DispatchToNextTick([Value]
    {
        if (g_RenderOffscreen != Value)
        {
            g_RenderOffscreen = Value;
        }
    });

    RequestUpdateResources();
}

void Renderer::SetUseDefaultSync(bool const Value)
{
    DispatchToNextTick([Value]
    {
        if (g_UseDefaultSync != Value)
        {
            g_UseDefaultSync = Value;
        }
    });

    RequestUpdateResources();
}

std::shared_ptr<Object> Renderer::GetObjectByID(std::uint32_t const ObjectID)
{
    return *std::ranges::find_if(GetObjects(),
                                 [ObjectID](std::shared_ptr<Object> const &ObjectIter)
                                 {
                                     return ObjectIter->GetID() == ObjectID;
                                 });
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

void Renderer::SaveOffscreenFrameToImage(strzilla::string_view const Path)
{
    ImageAllocation const &OffscreenImage = RenderCore::GetOffscreenImages().at(g_ImageIndex);
    SaveImageToFile(OffscreenImage.Image, Path, OffscreenImage.Extent);
}

std::vector<std::shared_ptr<Texture>> Renderer::LoadImages(std::vector<strzilla::string_view> &&Paths)
{
    if (std::empty(Paths))
    {
        return {};
    }

    std::vector<std::shared_ptr<Texture>> OutputImages;
    OutputImages.reserve(std::size(Paths));

    VkCommandPool                CommandPool { VK_NULL_HANDLE };
    std::vector<VkCommandBuffer> CommandBuffers { VK_NULL_HANDLE };

    auto const &                                                [QueueIndex, Queue] = GetGraphicsQueue();
    std::unordered_map<VkBuffer, VmaAllocation>                 BufferAllocations {};

    InitializeSingleCommandQueue(CommandPool, CommandBuffers, QueueIndex);
    {
        VkCommandBuffer &CommandBuffer = CommandBuffers.at(0U);

        for (strzilla::string_view const& PathIt : Paths)
        {
            TextureConstructionOutputParameters Output {};

            if (std::shared_ptr<Texture> NewTexture = ConstructTextureFromFile(PathIt, CommandBuffer, Output);
                NewTexture)
            {
                NewTexture->SetupTexture();

                OutputImages.push_back(std::move(NewTexture));
                BufferAllocations.emplace(std::move(Output.StagingBuffer), std::move(Output.StagingAllocation));
            }
        }
    }
    FinishSingleCommandQueue(Queue, CommandPool, CommandBuffers);

    VmaAllocator const &Allocator = GetAllocator();
    for (auto &[Buffer, Allocation] : BufferAllocations)
    {
        vmaDestroyBuffer(Allocator, Buffer, Allocation);
    }

    return OutputImages;
}
