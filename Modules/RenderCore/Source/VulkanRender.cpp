// Copyright Notice: [...]

#include "VulkanRender.h"
#include "Window.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanRender::VulkanRender()
    : m_Configurator(nullptr)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan render";
}

VulkanRender::~VulkanRender()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan render";
    Shutdown();
}

void VulkanRender::Initialize(GLFWwindow* const Window)
{
    if (IsInitialized() || !Window)
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing vulakn render";

    if (m_Configurator = std::make_unique<VulkanConfigurator>(); m_Configurator)
    {
        try
        {
            m_Configurator->Initialize(Window);
        }
        catch (const std::exception& Ex)
        {
            BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: Exception thrown: " << Ex.what();
        }
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Vulkan render initialized";
}

void VulkanRender::Shutdown()
{
    if (IsInitialized())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan render";

        try
        {
            m_Configurator->Shutdown();
        }
        catch (const std::exception& Ex)
        {
            BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: Exception thrown: " << Ex.what();
        }
    }
}

bool VulkanRender::IsInitialized() const
{
    return m_Configurator != nullptr && m_Configurator->IsInitialized();
}
