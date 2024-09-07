// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Texture;

import RenderCore.Types.Resource;
import RenderCore.Types.Material;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Texture : public Resource
    {
        bool m_RenderAsImage {false};
        VkDescriptorImageInfo m_ImageDescriptor {};
        VkDescriptorSet m_DescriptorSet {};
        std::vector<TextureType> m_Types {};

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
        [[nodiscard]] VkDescriptorSet const& GetDescriptorSet() const;

        void SetRenderAsImage(bool);
        void UpdateImageRendering();
    };
} // namespace RenderCore
