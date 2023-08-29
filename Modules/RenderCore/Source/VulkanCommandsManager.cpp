// Copyright Notice: [...]

#include "VulkanCommandsManager.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanCommandsManager::VulkanCommandsManager()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan commands manager";
}

VulkanCommandsManager::~VulkanCommandsManager()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan commands manager";
}
