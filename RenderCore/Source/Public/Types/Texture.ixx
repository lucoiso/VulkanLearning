// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Texture;

import RenderCore.Types.Resource;

export namespace RenderCore
{
    enum class TextureType : std::uint8_t
    {
        BaseColor,
        Normal,
        Occlusion,
        Emissive,
        MetallicRoughness,

        Count
    };

    class RENDERCOREMODULE_API Texture : public Resource
    {
        VkDescriptorImageInfo m_ImageDescriptor {};
        std::vector<TextureType> m_Types {};

    public:
        ~Texture() override = default;
         Texture()          = delete;

        Texture(std::uint32_t, strzilla::string_view);
        Texture(std::uint32_t, strzilla::string_view, strzilla::string_view);

        [[nodiscard]] inline std::vector<TextureType> const &GetTypes() const
        {
            return m_Types;
        }

        void SetTypes(std::vector<TextureType> const &);
        void AppendType(TextureType);

        void SetupTexture();

        [[nodiscard]] inline VkDescriptorImageInfo const &GetImageDescriptor() const
        {
            return m_ImageDescriptor;
        }
    };
} // namespace RenderCore
