// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <format>
#include <string>

module RenderCore.Factories.Texture;

import RenderCore.Runtime.Memory;
import RenderCore.Types.Texture;

using namespace RenderCore;

std::shared_ptr<Texture> RenderCore::ConstructTexture(TextureConstructionInputParameters const &Parameters,
                                                      TextureConstructionOutputParameters &     Output)
{
    if (std::empty(Parameters.Image.image))
    {
        return nullptr;
    }

    std::string const TextureName = std::format("{}_{:03d}", std::empty(Parameters.Image.name) ? "None" : Parameters.Image.name, Parameters.ID);
    auto              NewTexture  = std::shared_ptr<Texture>(new Texture { Parameters.ID, Parameters.Image.uri, TextureName }, TextureDeleter {});

    auto [Index, Buffer, Allocation] = AllocateTexture(Parameters.AllocationCmdBuffer,
                                                       std::data(Parameters.Image.image),
                                                       Parameters.Image.width,
                                                       Parameters.Image.height,
                                                       Parameters.Image.component == 3 ? VK_FORMAT_R8G8B8_UNORM : VK_FORMAT_R8G8B8A8_UNORM,
                                                       Parameters.Image.image.size());

    Output.StagingBuffer     = std::move(Buffer);
    Output.StagingAllocation = std::move(Allocation);

    NewTexture->SetBufferIndex(Index);

    return NewTexture;
}
