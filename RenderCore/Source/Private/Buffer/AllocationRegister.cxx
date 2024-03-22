// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <boost/log/trivial.hpp>
#include <sstream>
#include <ranges>
#include <vma/vk_mem_alloc.h>

module RenderCore.Management.AllocationRegister;

using namespace RenderCore;

import RuntimeInfo.Manager;

AllocationRegister& AllocationRegister::Get()
{
    static AllocationRegister Instance {};
    return Instance;
}

std::vector<AllocationRegisterData>& AllocationRegister::GetRegisterData()
{
    return m_RegisterData;
}

void AllocationRegister::RemoveElement(AllocationRegisterData const& Value)
{
    if (const auto MatchingAllocation = std::ranges::find(std::as_const(m_RegisterData), Value);
        MatchingAllocation != std::cend(m_RegisterData))
    {
        m_RegisterData.erase(MatchingAllocation);
    }
}

std::vector<AllocationRegisterData> const& AllocationRegister::GetRegister() const
{
    return m_RegisterData;
}

void AllocationRegister::AllocateDeviceMemoryCallback([[maybe_unused]] VmaAllocator const Allocator,
                                                      uint32_t const MemoryType,
                                                      VkDeviceMemory const Memory,
                                                      VkDeviceSize const AllocationSize,
                                                      void* const UserData)
{
    RuntimeInfo::Manager::Get().PushCallstack();

    std::stringstream MemAddress;
    MemAddress << std::hex << Memory;

    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Allocating device memory " << MemAddress.str() << " with size " << AllocationSize << " bytes.";

    AllocationRegisterData Data {MemoryType, Memory, AllocationSize, UserData};
    Get().RemoveElement(Data);
    Get().GetRegisterData().push_back(std::move(Data));
}

void AllocationRegister::FreeDeviceMemoryCallback([[maybe_unused]] VmaAllocator const Allocator,
                                                  [[maybe_unused]] uint32_t const MemoryType,
                                                  VkDeviceMemory const Memory,
                                                  VkDeviceSize const AllocationSize,
                                                  [[maybe_unused]] void* const UserData)
{
    RuntimeInfo::Manager::Get().PushCallstack();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Freeing device memory " << std::hex << Memory << " with size " << AllocationSize << " bytes.";

    AllocationRegisterData const Data {MemoryType, Memory, AllocationSize, UserData};
    Get().RemoveElement(Data);
}