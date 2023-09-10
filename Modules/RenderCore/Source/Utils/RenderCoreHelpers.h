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
#include <vulkan/vulkan_core.h>
#include <numbers>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

namespace RenderCore
{
#define RENDERCORE_CHECK_VULKAN_RESULT(INPUT_OPERATION)                                                                   \
    if (const VkResult OperationResult = INPUT_OPERATION; OperationResult != VK_SUCCESS)                                  \
    {                                                                                                                     \
        throw std::runtime_error("Vulkan operation failed with result: " + std::string(ResultToString(OperationResult))); \
    }

    static inline std::vector<const char *> GetGLFWExtensions()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting GLFW extensions";

        std::uint32_t GLFWExtensionsCount = 0u;
        const char **const GLFWExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionsCount);

        const std::vector<const char *> Output(GLFWExtensions, GLFWExtensions + GLFWExtensionsCount);

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Found extensions:";

        for (const char *const &ExtensionIter : Output)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: " << ExtensionIter;
        }

        return Output;
    }

    static inline VkExtent2D GetWindowExtent(GLFWwindow *const Window, const VkSurfaceCapabilitiesKHR &Capabilities)
    {
        std::int32_t Width = 0u;
        std::int32_t Height = 0u;
        glfwGetFramebufferSize(Window, &Width, &Height);

        VkExtent2D ActualExtent{
            .width = static_cast<std::uint32_t>(Width),
            .height = static_cast<std::uint32_t>(Height)};

        ActualExtent.width = std::clamp(ActualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
        ActualExtent.height = std::clamp(ActualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);

        return ActualExtent;
    }

    static inline std::vector<VkLayerProperties> GetAvailableInstanceLayers()
    {
        std::uint32_t LayersCount = 0u;
        RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceLayerProperties(&LayersCount, nullptr));
        
        std::vector<VkLayerProperties> Output;
        Output.reserve(LayersCount);
        RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceLayerProperties(&LayersCount, Output.data()));

        return Output;
    }

    static inline std::vector<const char*> GetAvailableInstanceLayersNames()
    {
        const std::vector<VkLayerProperties> AvailableInstanceLayers = GetAvailableInstanceLayers();
        
        std::vector<const char*> Output;
        Output.reserve(Output.size());
        for (const VkLayerProperties &LayerIter : AvailableInstanceLayers)
        {
            Output.push_back(LayerIter.layerName);
        }

        return Output;
    }

#ifdef _DEBUG
    static inline void ListAvailableInstanceLayers()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available instance layers...";

        const std::vector<VkLayerProperties> AvailableInstanceLayers = GetAvailableInstanceLayers();
        for (const VkLayerProperties &LayerIter : AvailableInstanceLayers)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Name: " << LayerIter.layerName;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Description: " << LayerIter.description;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Spec Version: " << LayerIter.specVersion;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Implementation Version: " << LayerIter.implementationVersion << std::endl;
        }
    }
#endif

    static inline std::vector<VkExtensionProperties> GetAvailableLayerExtensions(const char *LayerName)
    {
        std::uint32_t ExtensionCount = 0u;
        RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceExtensionProperties(LayerName, &ExtensionCount, nullptr));
        
        std::vector<VkExtensionProperties> Output;
        Output.reserve(ExtensionCount);
        RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceExtensionProperties(LayerName, &ExtensionCount, Output.data()));

        return Output;
    }

    static inline std::vector<const char*> GetAvailableLayerExtensionsNames(const char *LayerName)
    {
        const std::vector<VkExtensionProperties> AvailableLayerExtensions = GetAvailableLayerExtensions(LayerName);
        
        std::vector<const char*> Output;
        Output.reserve(Output.size());
        for (const VkExtensionProperties &ExtensionIter : AvailableLayerExtensions)
        {
            Output.push_back(ExtensionIter.extensionName);
        }

        return Output;
    }

#ifdef _DEBUG
    static inline void ListAvailableInstanceLayerExtensions(const char *LayerName)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available layer '" << LayerName << "' extensions...";

        const std::vector<VkExtensionProperties> AvailableLayerExtensions = GetAvailableLayerExtensions(LayerName);
        for (const VkExtensionProperties &ExtensionIter : AvailableLayerExtensions)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Name: " << ExtensionIter.extensionName;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Spec Version: " << ExtensionIter.specVersion << std::endl;
        }
    }
#endif

    constexpr std::array<VkVertexInputBindingDescription, 1> GetBindingDescriptors()
    {
        return {
            VkVertexInputBindingDescription{
                .binding = 0u,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}};
    }

    constexpr std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
    {
        return {
            VkVertexInputAttributeDescription{
                .location = 0u,
                .binding = 0u,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, Position)},
            VkVertexInputAttributeDescription{
                .location = 1u,
                .binding = 0u,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, Color)}};
    }

    constexpr void CreateTriangle(std::vector<Vertex> &Vertices, std::vector<std::uint16_t> &Indices)
    {
        Vertices.clear();
        Indices.clear();

        Vertices = {
            Vertex{.Position = glm::vec2(-0.5f, -0.5f), .Color = glm::vec3(1.f, 0.f, 0.f)},
            Vertex{.Position = glm::vec2(0.5f, -0.5f), .Color = glm::vec3(0.f, 1.f, 0.f)},
            Vertex{.Position = glm::vec2(0.f, 0.5f), .Color = glm::vec3(0.f, 0.f, 1.f)}};

        Indices = {0u, 1u, 2u};
    }

    constexpr void CreateSquare(std::vector<Vertex> &Vertices, std::vector<std::uint16_t> &Indices)
    {
        Vertices.clear();
        Indices.clear();

        Vertices = {
            Vertex{
                .Position = glm::vec2(-0.5f, -0.5f),
                .Color = glm::vec3(1.f, 0.f, 0.f)},
            Vertex{
                .Position = glm::vec2(0.5f, -0.5f),
                .Color = glm::vec3(0.f, 1.f, 0.f)},
            Vertex{
                .Position = glm::vec2(0.5f, 0.5f),
                .Color = glm::vec3(0.f, 0.f, 1.f)},
            Vertex{
                .Position = glm::vec2(-0.5f, 0.5f),
                .Color = glm::vec3(0.f, 0.f, 1.f)}};

        Indices = {0u, 1u, 2u, 2u, 3u, 0u};
    }

    constexpr void CreateCircle(std::vector<Vertex> &Vertices, std::vector<std::uint16_t> &Indices)
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

            Vertices.push_back(
                Vertex{.Position = glm::vec2(X, Y), .Color = glm::vec3(RedChannel, GreenChannel, BlueChannel)});

            Indices.emplace_back(0u);
            Indices.emplace_back(Iterator + 1u);
            Indices.emplace_back(Iterator + 2u);
        }
    }

    constexpr void CreateSphere(std::vector<Vertex> &Vertices, std::vector<std::uint16_t> &Indices)
    {
        const std::uint32_t Stacks = 20;
        const std::uint32_t Slices = 40;
        const float Radius = 1.0f;

        for (std::uint32_t Iterator1 = 0; Iterator1 <= Stacks; ++Iterator1)
        {
            const float Phi = std::numbers::pi * static_cast<float>(Iterator1) / static_cast<float>(Stacks);
            for (std::uint32_t Iterator2 = 0; Iterator2 <= Slices; ++Iterator2)
            {
                const float Theta = 2.f * std::numbers::pi * static_cast<float>(Iterator2) / static_cast<float>(Slices);
                const float X = std::sin(Phi) * std::cos(Theta);
                const float Y = std::sin(Phi) * std::sin(Theta);
                const float Z = std::cos(Phi);

                Vertex Vertex;
                Vertex.Position = glm::vec3(X * Radius, Y * Radius, Z * Radius);

                // Variando as cores para cada vértice
                Vertex.Color = glm::vec3(
                    std::abs(X),
                    std::abs(Y),
                    std::abs(Z)
                );

                Vertices.push_back(Vertex);
            }
        }

        for (std::uint32_t Iterator1 = 0; Iterator1 < Stacks; ++Iterator1)
        {
            for (std::uint32_t Iterator2 = 0; Iterator2 < Slices; ++Iterator2)
            {
                const std::uint32_t First = Iterator1 * (Slices + 1u) + Iterator2;
                const std::uint32_t Second = First + 1u;
                const std::uint32_t Third = (Iterator1 + 1u) * (Slices + 1u) + Iterator2;
                const std::uint32_t Fourth = Third + 1u;

                Indices.push_back(First);
                Indices.push_back(Second);
                Indices.push_back(Third);
                Indices.push_back(Second);
                Indices.push_back(Fourth);
                Indices.push_back(Third);
            }
        }
    }
}
#endif