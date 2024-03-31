// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <cstdint>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include "RenderCoreModule.hpp"

export module RenderCore.Subsystem.Allocation;

namespace RenderCore
{
    export struct AllocationSubsystemData
    {
        std::uint32_t  MemoryType {0U};
        VkDeviceMemory Memory {VK_NULL_HANDLE};
        VkDeviceSize   AllocationSize {0U};
        void          *UserData {nullptr};

        [[nodiscard]] bool operator==(AllocationSubsystemData const &Other) const
        {
            return Memory == Other.Memory && AllocationSize == Other.AllocationSize;
        }
    };

    export class RENDERCOREMODULE_API AllocationSubsystem
    {
        std::vector<AllocationSubsystemData> m_RegisterData {};

         AllocationSubsystem() = default;
        ~AllocationSubsystem() = default;

        [[nodiscard]] std::vector<AllocationSubsystemData> &GetRegisterData();
        void                                               RemoveElement(AllocationSubsystemData const &);

    public:
        static AllocationSubsystem &Get();

        [[nodiscard]] std::vector<AllocationSubsystemData> const &GetRegister() const;

        static void AllocateDeviceMemoryCallback(VmaAllocator, std::uint32_t, VkDeviceMemory, VkDeviceSize, void *);
        static void FreeDeviceMemoryCallback(VmaAllocator, std::uint32_t, VkDeviceMemory, VkDeviceSize, void *);
    };
} // namespace RenderCore
