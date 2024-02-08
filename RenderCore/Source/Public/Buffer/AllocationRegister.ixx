// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RenderCoreModule.hpp"
#include <cstdint>
#include <vector>
#include <vk_mem_alloc.h>

export module RenderCore.Management.AllocationRegister;

namespace RenderCore
{
    export struct AllocationRegisterData
    {
        std::uint32_t MemoryType {0U};
        VkDeviceMemory Memory {VK_NULL_HANDLE};
        VkDeviceSize AllocationSize {0U};
        void* UserData {nullptr};

        [[nodiscard]] bool operator==(AllocationRegisterData const& Other) const
        {
            return Memory == Other.Memory && AllocationSize == Other.AllocationSize;
        }
    };

    export class RENDERCOREMODULE_API AllocationRegister
    {
        std::vector<AllocationRegisterData> m_RegisterData {};

        AllocationRegister()  = default;
        ~AllocationRegister() = default;

        [[nodiscard]] std::vector<AllocationRegisterData>& GetRegisterData();
        void RemoveElement(AllocationRegisterData const&);

    public:
        static AllocationRegister& Get();

        [[nodiscard]] std::vector<AllocationRegisterData> const& GetRegister() const;

        static void AllocateDeviceMemoryCallback(VmaAllocator, std::uint32_t, VkDeviceMemory, VkDeviceSize, void*);
        static void FreeDeviceMemoryCallback(VmaAllocator, std::uint32_t, VkDeviceMemory, VkDeviceSize, void*);
    };
}// namespace RenderCore
