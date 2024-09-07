// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.UserInterface.Viewport;

import RenderCore.UserInterface.Control;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Viewport final : public RenderCore::Control
    {
        std::vector<VkDescriptorSet> m_ViewportDescriptorSets {};
        bool                         m_Open { false };

    public:
        explicit Viewport(Control *);
        ~Viewport() override;

        void TakeCameraControl(bool) const;
        [[nodiscard]] bool IsControllingCamera() const;

    protected:
        void Refresh() override;

        void PrePaint() override;

        void Paint() override;

        void PostPaint() override;
    };
} // namespace UserInterface
