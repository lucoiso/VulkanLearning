// Copyright Notice: [...]

#include "VulkanCommands.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanCommands::VulkanCommands()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan commands";
}

VulkanCommands::~VulkanCommands()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan commands";
}
