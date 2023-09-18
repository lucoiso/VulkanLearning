// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanRenderSubsystem.h"
#include "Utils/VulkanConstants.h"
#include <algorithm>
#include <boost/log/trivial.hpp>

using namespace RenderCore;

std::shared_ptr<VulkanRenderSubsystem> VulkanRenderSubsystem::m_SubsystemInstance(nullptr);

VulkanRenderSubsystem::VulkanRenderSubsystem()
    : m_Instance(VK_NULL_HANDLE)
    , m_Device(VK_NULL_HANDLE)
    , m_PhysicalDevice(VK_NULL_HANDLE)
    , m_Surface(VK_NULL_HANDLE)
    , m_RenderPass(VK_NULL_HANDLE)
    , m_PipelineCache(VK_NULL_HANDLE)
    , m_DescriptorPool(VK_NULL_HANDLE)
    , m_DeviceProperties()
    , m_QueueFamilyIndices({})
    , m_Queues({})
    , m_FrameIndex(0u)
    , m_DefaultShadersStageInfos({})
{
}

VulkanRenderSubsystem::~VulkanRenderSubsystem()
{
}

std::shared_ptr<VulkanRenderSubsystem> VulkanRenderSubsystem::Get()
{
    if (!m_SubsystemInstance)
    {
        m_SubsystemInstance = std::make_shared<VulkanRenderSubsystem>();
    }

    return m_SubsystemInstance;
}

void VulkanRenderSubsystem::SetInstance(const VkInstance &Instance)
{
    m_Instance = Instance;
}

void VulkanRenderSubsystem::SetDevice(const VkDevice &Device)
{
    m_Device = Device;
}

void VulkanRenderSubsystem::SetPhysicalDevice(const VkPhysicalDevice &PhysicalDevice)
{
    m_PhysicalDevice = PhysicalDevice;
}

void VulkanRenderSubsystem::SetSurface(const VkSurfaceKHR &Surface)
{
    m_Surface = Surface;
}

bool VulkanRenderSubsystem::SetDeviceProperties(const VulkanDeviceProperties &DeviceProperties)
{
    if (m_DeviceProperties != DeviceProperties)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Device properties changed. Updating...";    
        m_DeviceProperties = DeviceProperties;
        return false;
    }

    return m_DeviceProperties.IsValid();
}

void VulkanRenderSubsystem::SetQueueFamilyIndices(const std::vector<std::uint8_t> &QueueFamilyIndices)
{
    m_QueueFamilyIndices = QueueFamilyIndices;
}

void VulkanRenderSubsystem::SetDefaultShadersStageInfos(const std::vector<VkPipelineShaderStageCreateInfo> &DefaultShadersStageInfos)
{
    m_DefaultShadersStageInfos = DefaultShadersStageInfos;
}

void VulkanRenderSubsystem::SetRenderPass(const VkRenderPass &RenderPass)
{
    m_RenderPass = RenderPass;
}

void VulkanRenderSubsystem::SetPipelineCache(const VkPipelineCache &PipelineCache)
{
    m_PipelineCache = PipelineCache;
}

void VulkanRenderSubsystem::SetDescriptorPool(const VkDescriptorPool &DescriptorPool)
{
    m_DescriptorPool = DescriptorPool;
}

void VulkanRenderSubsystem::UpdateFrameIndex()
{
    m_FrameIndex = (m_FrameIndex + 1u) % g_MaxFramesInFlight;
}

std::uint8_t VulkanRenderSubsystem::RegisterQueue(const VkQueue &Queue, const std::uint8_t FamilyIndex, const VulkanQueueType Type)
{
    static std::atomic<std::uint8_t> ID(0u);
    const std::uint8_t QueueID = ID.fetch_add(1u);    
    m_Queues.emplace(QueueID, VulkanQueueHandle { .Handle = Queue, .FamilyIndex = FamilyIndex, .Type = Type });
    return QueueID;
}

void VulkanRenderSubsystem::UnregisterQueue(const std::uint8_t ID)
{
    m_Queues.erase(ID);
}

const VkInstance &VulkanRenderSubsystem::GetInstance() const
{
    return m_Instance;
}

const VkDevice &VulkanRenderSubsystem::GetDevice() const
{
    return m_Device;
}

const VkPhysicalDevice &VulkanRenderSubsystem::GetPhysicalDevice() const
{
    return m_PhysicalDevice;
}

const VkSurfaceKHR &VulkanRenderSubsystem::GetSurface() const
{
    return m_Surface;
}

const VulkanDeviceProperties &VulkanRenderSubsystem::GetDeviceProperties() const
{
    return m_DeviceProperties;
}

const std::vector<std::uint8_t> &VulkanRenderSubsystem::GetQueueFamilyIndices() const
{
    return m_QueueFamilyIndices;
}

const std::vector<std::uint32_t> RenderCore::VulkanRenderSubsystem::GetQueueFamilyIndices_u32() const
{
    std::vector<std::uint32_t> QueueFamilyIndices_u32(m_QueueFamilyIndices.size());
    std::transform(m_QueueFamilyIndices.begin(), m_QueueFamilyIndices.end(), QueueFamilyIndices_u32.begin(), [](const std::uint8_t &Index) { return static_cast<std::uint32_t>(Index); });
    return QueueFamilyIndices_u32;
}

const VkQueue &VulkanRenderSubsystem::GetQueueFromID(const uint8_t ID) const
{
    return m_Queues.at(ID).Handle;
}

const VkQueue &VulkanRenderSubsystem::GetQueueFromType(const VulkanQueueType Type) const
{
    for (const auto &[_, Queue] : m_Queues)
    {
        if (Queue.Type == Type)
        {
            return Queue.Handle;
        }
    }

    return m_Queues.at(0u).Handle;
}

const std::uint8_t VulkanRenderSubsystem::GetQueueFamilyIndexFromID(const uint8_t ID) const
{
    return m_Queues.at(ID).FamilyIndex;
}

const std::uint8_t VulkanRenderSubsystem::GetQueueFamilyIndexFromType(const VulkanQueueType Type) const
{
    for (const auto &[_, Queue] : m_Queues)
    {
        if (Queue.Type == Type)
        {
            return Queue.FamilyIndex;
        }
    }

    return m_Queues.at(0u).FamilyIndex;
}

const VulkanQueueType VulkanRenderSubsystem::GetQueueTypeFromID(const uint8_t ID) const
{
    return m_Queues.at(ID).Type;
}

const std::uint8_t VulkanRenderSubsystem::GetFrameIndex() const
{
    return m_FrameIndex;
}

const std::vector<VkPipelineShaderStageCreateInfo> &VulkanRenderSubsystem::GetDefaultShadersStageInfos() const
{
    return m_DefaultShadersStageInfos;
}

const std::uint8_t VulkanRenderSubsystem::GetMinImageCount() const
{
    const bool bSupportsTripleBuffering = m_DeviceProperties.Capabilities.minImageCount < 3u && m_DeviceProperties.Capabilities.maxImageCount >= 3u;
    return bSupportsTripleBuffering ? 3u : m_DeviceProperties.Capabilities.minImageCount;
}

const VkRenderPass &VulkanRenderSubsystem::GetRenderPass() const
{
    return m_RenderPass;
}

const VkPipelineCache &VulkanRenderSubsystem::GetPipelineCache() const
{
    return m_PipelineCache;
}

const VkDescriptorPool &VulkanRenderSubsystem::GetDescriptorPool() const
{
    return m_DescriptorPool;
}
