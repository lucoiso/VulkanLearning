// Copyright Notice: [...]

#include "VulkanPipeline.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanPipeline::VulkanPipeline()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan pipeline";
}

VulkanPipeline::~VulkanPipeline()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan pipeline";
}
