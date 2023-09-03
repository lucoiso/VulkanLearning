// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#ifndef RENDERCOREHELPERS_H
#define RENDERCOREHELPERS_H

#pragma once

#include "Utils/VulkanConstants.h"
#include "Utils/VulkanEnumConverter.h"
#include "Types/VulkanVertex.h"
#include <boost/log/trivial.hpp>
#include <vulkan/vulkan.h>
#include <numbers>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

namespace RenderCore
{
    static inline std::vector<const char*> GetGLFWExtensions()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting GLFW extensions";

        std::uint32_t GLFWExtensionsCount = 0u;
        const char** const GLFWExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionsCount);

        const std::vector<const char*> Output(GLFWExtensions, GLFWExtensions + GLFWExtensionsCount);

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Found extensions:";

        for (const char* const& ExtensionIter : Output)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: " << ExtensionIter;
        }

        return Output;
    }

    static inline VkExtent2D GetWindowExtent(GLFWwindow* const Window, const VkSurfaceCapabilitiesKHR& Capabilities)
    {
        std::int32_t Width = 0u;
        std::int32_t Height = 0u;
        glfwGetFramebufferSize(Window, &Width, &Height);

        VkExtent2D ActualExtent{
            .width = static_cast<std::uint32_t>(Width),
            .height = static_cast<std::uint32_t>(Height)
        };

        ActualExtent.width = std::clamp(ActualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
        ActualExtent.height = std::clamp(ActualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);

        return ActualExtent;
    }

    static inline std::vector<VkLayerProperties> GetAvailableValidationLayers()
    {
#ifdef NDEBUG
        return {};
#else
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting available validation layers";

        std::vector<VkLayerProperties> Output;

        if (g_ValidationLayers.empty())
        {
            return Output;
        }

        std::uint32_t LayersCount = 0u;
        if (vkEnumerateInstanceLayerProperties(&LayersCount, nullptr) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to enumerate Vulkan Layers.");
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Found " << LayersCount << " validation layers";

        if (LayersCount == 0u)
        {
            return Output;
        }

        Output.resize(LayersCount);
        if (vkEnumerateInstanceLayerProperties(&LayersCount, Output.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to get Vulkan Layers properties.");
        }

        for (const VkLayerProperties& LayerIter : Output)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Name: " << LayerIter.layerName;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Description: " << LayerIter.description;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Spec Version: " << LayerIter.specVersion;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Implementation Version: " << LayerIter.implementationVersion << std::endl;
        }

        return Output;
#endif
    }

    constexpr void CreateTriangle(std::vector<Vertex>& Vertices, std::vector<std::uint32_t>& Indices)
    {
        Vertices.clear();
        Indices.clear();

        Vertices = {
            Vertex{ .Position = { -0.5f, -0.5f }, .Color = { 10.f, 00.f, 00.f } },
            Vertex{ .Position = { 0.5f, -0.5f }, .Color = { 00.f, 10.f, 00.f } },
            Vertex{ .Position = { 00.f, 0.5f }, .Color = { 00.f, 00.f, 10.f } }
        };

        Indices = { 0u, 1u, 2u };
    }

    constexpr void CreateSquare(std::vector<Vertex>& Vertices, std::vector<std::uint32_t>& Indices)
    {
        Vertices.clear();
        Indices.clear();

        Vertices = {
            Vertex {
                .Position = { -0.5f, -0.5f, 0.f },
                .Color = {10.f, 00.f, 00.f }
            },
            Vertex {
                .Position = { 0.5f, -0.5f, 0.f },
                .Color = {00.f, 10.f, 00.f}
            },
            Vertex {
                .Position = { 0.5f, 0.5f, 0.f },
                .Color = {00.f, 00.f, 10.f}
            },
            Vertex {
                .Position = { -0.5f, 0.5f, 0.f },
                .Color = { 00.f, 00.f, 10.f }
            }
        };

        Indices = { 0u, 1u, 2u, 2u, 3u, 0u };
    }

    constexpr void CreateCircle(std::vector<Vertex>& Vertices, std::vector<std::uint32_t>& Indices)
    {
        Vertices.clear();
        Indices.clear();

        constexpr std::uint32_t CircleResolution = 100u;
        constexpr float CircleRadius = 0.5f;

        Vertices.reserve(CircleResolution + 1u);
        Indices.reserve(CircleResolution * 3u);

        for (std::uint32_t Iterator = 0u; Iterator < CircleResolution; ++Iterator)
        {
            const float Angle = 2.f * std::numbers::pi * Iterator / CircleResolution;
            const float X = CircleRadius * std::cos(Angle);
            const float Y = CircleRadius * std::sin(Angle);

            const float RedChannel = std::clamp(1.f - X, 0.f, 1.f);
            const float GreenChannel = std::clamp(Y + 0.5f, 0.f, 1.f);
            const float BlueChannel = std::clamp(X + 0.5f, 0.f, 1.f);

            Vertices.emplace_back(Vertex{ .Position = { X, Y, 0.f }, .Color = { RedChannel, GreenChannel, BlueChannel } });

            Indices.emplace_back(0u);
            Indices.emplace_back(Iterator + 1u);
            Indices.emplace_back(Iterator + 2u);
        }
    }
}

#define RENDERCORE_CHECK_VULKAN_RESULT(INPUT_OPERATION) \
if (const VkResult OperationResult = INPUT_OPERATION; OperationResult != VK_SUCCESS) \
{ \
    throw std::runtime_error("Vulkan operation failed with result: " + std::string(ResultToString(OperationResult))); \
}

#endif