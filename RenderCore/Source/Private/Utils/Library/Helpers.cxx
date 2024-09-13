// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Utils.Helpers;

import RenderCore.Types.Vertex;
import RenderCore.Utils.EnumConverter;

using namespace RenderCore;

std::string ExtractFunctionName(std::string const &FunctionName)
{
    constexpr auto Pattern = R"(\b([a-zA-Z_][a-zA-Z0-9_]*)\b(?=\s*\())";

    if (std::smatch Match;
        std::regex_search(FunctionName, Match, std::regex { Pattern }))
    {
        return Match.str();
    }

    return {};
}

std::string ExtractFileName(strzilla::string_view const &FileName)
{
    return std::filesystem::path(std::data(FileName)).filename().string();
}

void RenderCore::EmitFatalError(strzilla::string_view const Message, std::source_location const &Location)
{
    BOOST_LOG_TRIVIAL(fatal) << std::format("[{}:{}:{}:{}] {}",
                                            ExtractFileName(Location.file_name()),
                                            ExtractFunctionName(Location.function_name()),
                                            Location.line(),
                                            Location.column(),
                                            std::data(Message));

    std::terminate();
}

void RenderCore::CheckVulkanResult(VkResult const InputOperation, std::source_location const &Location)
{
    if (InputOperation != VK_SUCCESS)
    {
        EmitFatalError(std::format("Vulkan operation failed with result {}", ResultToString(InputOperation)), Location);
    }
}

bool RenderCore::operator==(VkExtent2D const Lhs, VkExtent2D const Rhs)
{
    return Lhs.width == Rhs.width && Lhs.height == Rhs.height;
}

VkExtent2D RenderCore::GetFramebufferSize(GLFWwindow *const Window)
{
    std::int32_t Width  = 0U;
    std::int32_t Height = 0U;
    glfwGetFramebufferSize(Window, &Width, &Height);

    return VkExtent2D { .width = static_cast<std::uint32_t>(Width), .height = static_cast<std::uint32_t>(Height) };
}

VkExtent2D RenderCore::GetWindowExtent(GLFWwindow *const Window, VkSurfaceCapabilitiesKHR const &Capabilities)
{
    VkExtent2D ActualExtent = GetFramebufferSize(Window);

    ActualExtent.width  = std::clamp(ActualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
    ActualExtent.height = std::clamp(ActualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);

    return ActualExtent;
}

std::vector<strzilla::string> RenderCore::GetGLFWExtensions()
{
    std::uint32_t   GLFWExtensionsCount = 0U;
    char const **   GLFWExtensions      = glfwGetRequiredInstanceExtensions(&GLFWExtensionsCount);
    std::span const GLFWExtensionsSpan(GLFWExtensions, GLFWExtensionsCount);

    std::vector<strzilla::string> Output {};
    Output.reserve(GLFWExtensionsCount);

    if (!std::empty(GLFWExtensionsSpan))
    {
        for (char const *const &ExtensionIter : GLFWExtensionsSpan)
        {
            Output.emplace_back(ExtensionIter);
        }
    }
    else
    {
        Output.emplace_back("VK_KHR_surface");

        #ifdef WIN32
        Output.emplace_back("VK_KHR_win32_surface");
        #elif __linux__
        Output.emplace_back("VK_KHR_xcb_surface");
        #elif __APPLE__
        Output.emplace_back("VK_KHR_macos_surface");
        elif __ANDROID__
        Output.emplace_back("VK_KHR_android_surface");
        #endif
    }

    return Output;
}

std::vector<VkLayerProperties> RenderCore::GetAvailableInstanceLayers()
{
    std::uint32_t LayersCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceLayerProperties(&LayersCount, nullptr));

    std::vector Output(LayersCount, VkLayerProperties());
    CheckVulkanResult(vkEnumerateInstanceLayerProperties(&LayersCount, std::data(Output)));

    return Output;
}

std::vector<strzilla::string> RenderCore::GetAvailableInstanceLayersNames()
{
    std::vector<strzilla::string> Output;
    for (auto const &[LayerName, SpecVer, ImplVer, Descr] : GetAvailableInstanceLayers())
    {
        Output.emplace_back(LayerName);
    }

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailableInstanceExtensions()
{
    std::uint32_t ExtensionCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    std::vector Output(ExtensionCount, VkExtensionProperties());
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, std::data(Output)));

    return Output;
}

std::vector<strzilla::string> RenderCore::GetAvailableInstanceExtensionsNames()
{
    std::vector<strzilla::string> Output;
    for (auto const &[ExtName, SpecVer] : GetAvailableInstanceExtensions())
    {
        Output.emplace_back(ExtName);
    }

    return Output;
}

VkVertexInputBindingDescription RenderCore::GetBindingDescriptors(std::uint32_t const InBinding)
{
    return VkVertexInputBindingDescription { .binding = InBinding, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };
}

std::vector<VkVertexInputAttributeDescription> RenderCore::GetAttributeDescriptions(std::uint32_t const                                   InBinding,
                                                                                    std::vector<VkVertexInputAttributeDescription> const &Attributes)
{
    std::vector Output(std::cbegin(Attributes), std::cend(Attributes));

    std::uint32_t AttributeLocation { 0U };
    for (auto &[Location, Binding, Format, Offset] : Output)
    {
        Binding  = InBinding;
        Location = AttributeLocation;

        ++AttributeLocation;
    }

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailableInstanceLayerExtensions(strzilla::string_view const LayerName)
{
    if (std::vector<strzilla::string> const AvailableLayers = GetAvailableInstanceLayersNames();
        std::ranges::find(AvailableLayers, std::data(LayerName)) == std::cend(AvailableLayers))
    {
        return {};
    }

    std::uint32_t ExtensionCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(std::data(LayerName), &ExtensionCount, nullptr));

    std::vector Output(ExtensionCount, VkExtensionProperties());
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(std::data(LayerName), &ExtensionCount, std::data(Output)));

    return Output;
}

std::vector<strzilla::string> RenderCore::GetAvailableInstanceLayerExtensionsNames(strzilla::string_view const LayerName)
{
    std::vector<strzilla::string> Output;
    for (auto const &[ExtName, SpecVer] : GetAvailableInstanceLayerExtensions(LayerName))
    {
        Output.emplace_back(ExtName);
    }

    return Output;
}

void RenderCore::DispatchQueue(std::queue<std::function<void()>> &Queue)
{
    while (!std::empty(Queue))
    {
        auto &Dispatch = Queue.front();
        Dispatch();
        Queue.pop();
    }
}