// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>

export module RenderCore.Types.Texture;

import RenderCore.Types.Resource;
import RenderCore.Types.Material;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Texture : public Resource
    {
        std::vector<TextureType> m_Types {};
        VkDescriptorImageInfo    m_ImageDescriptor {};

    public:
        ~Texture() override = default;
         Texture()          = delete;

        Texture(std::uint32_t, strzilla::string_view);
        Texture(std::uint32_t, strzilla::string_view, strzilla::string_view);

        [[nodiscard]] std::vector<TextureType> GetTypes() const;
        void                                   SetTypes(std::vector<TextureType> const &);
        void                                   AppendType(TextureType);

        void SetupTexture();

        [[nodiscard]] VkDescriptorImageInfo GetImageDescriptor() const;
    };
} // namespace RenderCore
