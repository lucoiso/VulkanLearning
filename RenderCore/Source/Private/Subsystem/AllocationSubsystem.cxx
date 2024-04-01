// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <ranges>
#include <sstream>
#include <vma/vk_mem_alloc.h>

module RenderCore.Subsystem.Allocation;

using namespace RenderCore;

import RuntimeInfo.Manager;

AllocationSubsystem &AllocationSubsystem::Get()
{
    static AllocationSubsystem Instance {};
    return Instance;
}

std::vector<AllocationSubsystemData> &AllocationSubsystem::GetRegisterData()
{
    return m_RegisterData;
}

void AllocationSubsystem::RemoveElement(AllocationSubsystemData const &Value)
{
    if (const auto MatchingAllocation = std::ranges::find(std::as_const(m_RegisterData), Value); MatchingAllocation != std::cend(m_RegisterData))
    {
        m_RegisterData.erase(MatchingAllocation);
    }
}

std::vector<AllocationSubsystemData> const &AllocationSubsystem::GetRegister() const
{
    return m_RegisterData;
}

void AllocationSubsystem::AllocateDeviceMemoryCallback([[maybe_unused]] VmaAllocator const Allocator,
                                                       uint32_t const                      MemoryType,
                                                       VkDeviceMemory const                Memory,
                                                       VkDeviceSize const                  AllocationSize,
                                                       void *const                         UserData)
{
    AllocationSubsystemData Data {MemoryType, Memory, AllocationSize, UserData};
    Get().RemoveElement(Data);
    Get().GetRegisterData().push_back(std::move(Data));
}

void AllocationSubsystem::FreeDeviceMemoryCallback([[maybe_unused]] VmaAllocator const Allocator,
                                                   [[maybe_unused]] uint32_t const     MemoryType,
                                                   VkDeviceMemory const                Memory,
                                                   VkDeviceSize const                  AllocationSize,
                                                   [[maybe_unused]] void *const        UserData)
{
    AllocationSubsystemData const Data {MemoryType, Memory, AllocationSize, UserData};
    Get().RemoveElement(Data);
}
