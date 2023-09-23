// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include "Types/QueueType.h"
#include "Types/DeviceProperties.h"
#include <volk.h>
#include <vector>
#include <unordered_map>
#include <memory>

namespace RenderCore
{
    class VulkanRenderSubsystem
    {
    public:
        VulkanRenderSubsystem(const VulkanRenderSubsystem &) = delete;
        VulkanRenderSubsystem &operator=(const VulkanRenderSubsystem &) = delete;

        VulkanRenderSubsystem();
        ~VulkanRenderSubsystem();

        static std::shared_ptr<VulkanRenderSubsystem> Get();

        void SetInstance(const VkInstance &Instance);
        void SetDevice(const VkDevice &Device);
        void SetPhysicalDevice(const VkPhysicalDevice &PhysicalDevice);
        void SetSurface(const VkSurfaceKHR &Surface);
        bool SetDeviceProperties(const VulkanDeviceProperties &DeviceProperties);
        void SetQueueFamilyIndices(const std::vector<std::uint8_t> &QueueFamilyIndices);
        void SetDefaultShadersStageInfos(const std::vector<VkPipelineShaderStageCreateInfo> &DefaultShadersStageInfos);
        void SetMinImageCount(const std::uint8_t MinImageCount);
        void SetRenderPass(const VkRenderPass &RenderPass);
        void SetPipelineCache(const VkPipelineCache &PipelineCache);
        void SetDescriptorPool(const VkDescriptorPool &DescriptorPool);
        
        void UpdateFrameIndex();

        std::uint8_t RegisterQueue(const VkQueue &Queue, const std::uint8_t FamilyIndex, const VulkanQueueType Type);
        void UnregisterQueue(const std::uint8_t ID);

        [[nodiscard]] const VkInstance &GetInstance() const;
        [[nodiscard]] const VkDevice &GetDevice() const;
        [[nodiscard]] const VkPhysicalDevice &GetPhysicalDevice() const;
        [[nodiscard]] const VkSurfaceKHR &GetSurface() const;
        [[nodiscard]] const VulkanDeviceProperties &GetDeviceProperties() const;
        [[nodiscard]] const std::vector<std::uint8_t> &GetQueueFamilyIndices() const;
        [[nodiscard]] const std::vector<std::uint32_t> GetQueueFamilyIndices_u32() const;
        [[nodiscard]] const VkQueue &GetQueueFromID(const uint8_t ID) const;
        [[nodiscard]] const VkQueue &GetQueueFromType(const VulkanQueueType Type) const;
        [[nodiscard]] const std::uint8_t GetQueueFamilyIndexFromID(const uint8_t ID) const;
        [[nodiscard]] const std::uint8_t GetQueueFamilyIndexFromType(const VulkanQueueType Type) const;
        [[nodiscard]] const VulkanQueueType GetQueueTypeFromID(const uint8_t ID) const;
        [[nodiscard]] const std::uint8_t GetFrameIndex() const;
        [[nodiscard]] const std::vector<VkPipelineShaderStageCreateInfo> &GetDefaultShadersStageInfos() const;
        [[nodiscard]] const std::uint8_t GetMinImageCount() const;
        [[nodiscard]] const VkRenderPass &GetRenderPass() const;
        [[nodiscard]] const VkPipelineCache &GetPipelineCache() const;
        [[nodiscard]] const VkDescriptorPool &GetDescriptorPool() const;

    private:
        static std::shared_ptr<VulkanRenderSubsystem> m_SubsystemInstance;

        const VkInstance *m_Instance;
        const VkDevice *m_Device;
        const VkPhysicalDevice *m_PhysicalDevice;
        const VkSurfaceKHR *m_Surface;
        const VkRenderPass *m_RenderPass;
        const VkPipelineCache *m_PipelineCache;
        const VkDescriptorPool *m_DescriptorPool;

        VulkanDeviceProperties m_DeviceProperties;
        std::vector<std::uint8_t> m_QueueFamilyIndices;

        struct VulkanQueueHandle
        {
            VkQueue Handle;
            std::uint8_t FamilyIndex;
            VulkanQueueType Type;
        };

        std::unordered_map<std::uint8_t, VulkanQueueHandle> m_Queues;
        std::uint8_t m_FrameIndex;
        std::vector<VkPipelineShaderStageCreateInfo> m_DefaultShadersStageInfos;
        std::uint8_t m_MinImageCount;
    };
}