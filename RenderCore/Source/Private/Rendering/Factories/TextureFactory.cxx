// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <stb_image.h>

module RenderCore.Factories.Texture;

import RenderCore.Runtime.Scene;
import RenderCore.Runtime.Memory;
import RenderCore.Types.Texture;

using namespace RenderCore;

std::shared_ptr<Texture> RenderCore::ConstructTexture(TextureConstructionInputParameters const &Parameters,
                                                      TextureConstructionOutputParameters &     Output)
{
    EASY_FUNCTION(profiler::colors::Red50);

    if (std::empty(Parameters.Image.image))
    {
        return nullptr;
    }

    strzilla::string const TextureName = std::format("{}_{:03d}", std::empty(Parameters.Image.name) ? "None" : Parameters.Image.name, Parameters.ID);
    auto              NewTexture  = std::shared_ptr<Texture>(new Texture { Parameters.ID, Parameters.Image.uri, TextureName }, TextureDeleter {});

    auto [Index, Buffer, Allocation] = AllocateTexture(Parameters.AllocationCmdBuffer,
                                                       std::data(Parameters.Image.image),
                                                       Parameters.Image.width,
                                                       Parameters.Image.height,
                                                       Parameters.Image.component == 3 ? VK_FORMAT_R8G8B8_UNORM : VK_FORMAT_R8G8B8A8_UNORM,
                                                       std::size(Parameters.Image.image));

    Output.StagingBuffer     = std::move(Buffer);
    Output.StagingAllocation = std::move(Allocation);

    NewTexture->SetBufferIndex(Index);

    return NewTexture;
}

std::shared_ptr<Texture> RenderCore::ConstructTextureFromFile(strzilla::string_view const &Path, VkCommandBuffer& CommandBuffer, TextureConstructionOutputParameters &Output)
{
    if (std::empty(Path) || !std::filesystem::exists(std::data(Path)))
    {
        return nullptr;
    }

    std::int32_t Width    = -1;
    std::int32_t Height   = -1;
    std::int32_t Channels = -1;

    stbi_uc const* const ImagePixels = stbi_load(std::data(Path), &Width, &Height, &Channels, STBI_rgb_alpha);
    auto const AllocationSize = static_cast<VkDeviceSize>(Width) * static_cast<VkDeviceSize>(Height) * 4U;

    if (ImagePixels == nullptr)
    {
        return nullptr;
    }

    tinygltf::Image ImageData;
    ImageData.name = std::data(Path);
    ImageData.width = Width;
    ImageData.height = Height;
    ImageData.component = Channels;
    ImageData.image = std::vector(ImagePixels, ImagePixels + AllocationSize);
    ImageData.uri = std::data(Path);

    return ConstructTexture(TextureConstructionInputParameters{
        .ID = FetchID(),
        .Image = ImageData,
        .AllocationCmdBuffer = CommandBuffer,
    }, Output);
}
