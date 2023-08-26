// Copyright Notice: [...]

#include "VulkanBufferManager.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanBufferManager::VulkanBufferManager()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan buffer manager";
}

VulkanBufferManager::~VulkanBufferManager()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan buffer manager";
}
