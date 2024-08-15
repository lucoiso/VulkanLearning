// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <glm/ext.hpp>

export module RenderCore.Types.Illumination;

import RenderCore.Types.Transform;
import RenderCore.Types.Allocation;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Illumination
    {
        mutable bool                                        m_IsRenderDirty { true };
        float                                               m_Intensity { 1.F };
        float                                               m_Ambient { 1.F };
        glm::vec3                                           m_Position { 100.F, 100.F, 100.F };
        glm::vec3                                           m_Color { 1.F, 1.F, 1.F };
        std::pair<BufferAllocation, VkDescriptorBufferInfo> m_UniformBufferAllocation {};

    public:
        Illumination() = default;

        void                           SetPosition(glm::vec3 const &);
        [[nodiscard]] glm::vec3 const &GetPosition() const;

        void                           SetColor(glm::vec3 const &);
        [[nodiscard]] glm::vec3 const &GetColor() const;

        void                SetIntensity(float);
        [[nodiscard]] float GetIntensity() const;

        void                SetAmbient(float);
        [[nodiscard]] float GetAmbient() const;

        [[nodiscard]] bool IsRenderDirty() const;
        void               SetRenderDirty(bool) const;
    };
} // namespace RenderCore
