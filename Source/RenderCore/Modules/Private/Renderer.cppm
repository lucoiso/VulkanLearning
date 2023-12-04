// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

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

module RenderCore.Renderer;

import <vector>;
import <filesystem>;

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

using namespace RenderCore;

VkInstance g_Instance {VK_NULL_HANDLE};
Timer::Manager g_RenderTimerManager {};

#ifdef _DEBUG
VkDebugUtilsMessengerEXT g_DebugMessenger {VK_NULL_HANDLE};
#endif

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

    std::vector Layers(std::cbegin(g_RequiredInstanceLayers), std::cend(g_RequiredInstanceLayers));
    std::vector<char const*> Extensions = GetGLFWExtensions();
    Extensions.insert(std::cend(Extensions), std::cbegin(g_RequiredInstanceExtensions), std::cend(g_RequiredInstanceExtensions));

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
}

void InitializeRenderCore(GLFWwindow* const Window, VkSurfaceKHR const& VulkanSurface)
{
    PickPhysicalDevice(VulkanSurface);
    CreateLogicalDevice(VulkanSurface);
    volkLoadDevice(GetLogicalDevice());
    UpdateDeviceProperties(Window, VulkanSurface);
}

void Renderer::DrawFrame(GLFWwindow* const Window)
{
    if (!IsInitialized())
    {
        return;
    }

    if (Window == nullptr)
    {
        throw std::runtime_error("Window is invalid");
    }

    RemoveInvalidObjects();

    constexpr EngineCoreStateFlags InvalidStatesToRender = EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE
                                                           | EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION
                                                           | EngineCoreStateFlags::PENDING_RESOURCES_CREATION
                                                           | EngineCoreStateFlags::PENDING_PIPELINE_REFRESH;

    if (HasAnyFlag(m_StateFlags, InvalidStatesToRender))
    {
        if (HasFlag(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION))
        {
            DestroyCommandsSynchronizationObjects();

            m_BufferManager.DestroyBufferResources(false);
            m_PipelineManager.ReleaseDynamicPipelineResources();

            RemoveFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
            AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION);
        }

        if (HasFlag(m_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE))
        {
            if (UpdateDeviceProperties(Window, m_BufferManager.GetSurface()))
            {
                BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Device properties updated, starting to draw frames with new properties";
                RemoveFlags(m_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
            }
        }

        if (HasFlag(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION) && !HasFlag(m_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE))
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Refreshing resources...";

            m_BufferManager.CreateSwapChain();
            m_BufferManager.CreateDepthResources();
            CreateCommandsSynchronizationObjects();

            RemoveFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION);
            AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_PIPELINE_REFRESH);
        }

        if (HasFlag(m_StateFlags, EngineCoreStateFlags::PENDING_PIPELINE_REFRESH))
        {
            m_PipelineManager.CreateDescriptorSetLayout();
            m_PipelineManager.CreateGraphicsPipeline();

            m_BufferManager.CreateFrameBuffers(m_PipelineManager.GetRenderPass());

            m_PipelineManager.CreateDescriptorPool(m_BufferManager.GetClampedNumAllocations());

            auto MeshData = m_BufferManager.GetAllocatedObjects();
            m_PipelineManager.CreateDescriptorSets(MeshData);

            RemoveFlags(m_StateFlags, EngineCoreStateFlags::PENDING_PIPELINE_REFRESH);
        }
    }

    if (!HasAnyFlag(m_StateFlags, InvalidStatesToRender))
    {
        if (std::optional<std::int32_t> const ImageIndice = RequestImageIndex();
            ImageIndice.has_value())
        {
            std::lock_guard Lock(m_ObjectsMutex);

            RecordCommandBuffers(ImageIndice.value(), m_BufferManager, m_PipelineManager, GetObjects());
            SubmitCommandBuffers();
            PresentFrame(ImageIndice.value(), m_BufferManager.GetSwapChain());
        }
    }
}

std::optional<std::int32_t> Renderer::RequestImageIndex()
{
    if (!GetDeviceProperties().IsValid())
    {
        AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
        AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);

        return std::nullopt;
    }
    RemoveFlags(m_StateFlags, EngineCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);

    std::optional<std::int32_t> const Output = RequestSwapChainImage(m_BufferManager.GetSwapChain());

    if (!Output.has_value())
    {
        AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }
    else
    {
        RemoveFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
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

    if (DeltaTime <= 0.0f)
    {
        return;
    }

    m_DeltaTime = DeltaTime;

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
    m_BufferManager.CreateVulkanSurface(Window);
    InitializeRenderCore(Window, m_BufferManager.GetSurface());

    m_BufferManager.CreateMemoryAllocator();
    auto const _ = CompileDefaultShaders();
    m_PipelineManager.CreateRenderPass();
    InitializeImGui(Window, m_PipelineManager);

    AddFlags(m_StateFlags, EngineCoreStateFlags::INITIALIZED);
    AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION);

    return UpdateDeviceProperties(Window, m_BufferManager.GetSurface()) && IsInitialized();
}

void Renderer::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    RemoveFlags(m_StateFlags, EngineCoreStateFlags::INITIALIZED);

    ReleaseImGuiResources();
    ReleaseShaderResources();
    ReleaseCommandsResources();
    m_BufferManager.ReleaseBufferResources();
    m_PipelineManager.ReleasePipelineResources();
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

    vkDestroyInstance(g_Instance, nullptr);
    g_Instance = VK_NULL_HANDLE;

    m_Objects.clear();
}

bool Renderer::IsInitialized() const
{
    return HasFlag(m_StateFlags, EngineCoreStateFlags::INITIALIZED);
}

std::vector<std::uint32_t> Renderer::LoadScene(std::string_view const& ObjectPath)
{
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

    std::vector<Object> LoadedObjects = m_BufferManager.AllocateScene(ObjectPath);
    std::vector<std::uint32_t> Output;
    Output.reserve(std::size(LoadedObjects));

    for (Object& ObjectIter: LoadedObjects)
    {
        m_Objects.emplace_back(std::make_shared<Object>(std::move(ObjectIter)));
        Output.push_back(ObjectIter.GetID());
    }

    AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
    AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_CREATION);

    return Output;
}

void Renderer::UnloadScene(std::vector<std::uint32_t> const& ObjectIDs)
{
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

    AddFlags(m_StateFlags, EngineCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
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

void InitializeEngineCore()
{
}

bool IsEngineCoreInitialized()
{
    return true;
}