// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Illumination;

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

        inline void SetPosition(glm::vec3 const &Value)
        {
            if (m_Position != Value)
            {
                m_Position      = Value;
                m_IsRenderDirty = true;
            }
        }

        [[nodiscard]] inline glm::vec3 const &GetPosition() const
        {
            return m_Position;
        }

        inline void SetColor(glm::vec3 const &Value)
        {
            if (m_Color != Value)
            {
                m_Color         = Value;
                m_IsRenderDirty = true;
            }
        }

        [[nodiscard]] inline glm::vec3 const &GetColor() const
        {
            return m_Color;
        }

        inline void SetIntensity(float const Value)
        {
            if (m_Intensity != Value)
            {
                m_Intensity     = Value;
                m_IsRenderDirty = true;
            }
        }

        [[nodiscard]] inline float GetIntensity() const
        {
            return m_Intensity;
        }

        inline void SetAmbient(float const Value)
        {
            if (m_Ambient != Value)
            {
                m_Ambient       = Value;
                m_IsRenderDirty = true;
            }
        }

        [[nodiscard]] inline float GetAmbient() const
        {
            return m_Ambient;
        }

        [[nodiscard]] inline bool IsRenderDirty() const
        {
            return m_IsRenderDirty;
        }

        inline void SetRenderDirty(bool const Value) const
        {
            m_IsRenderDirty = Value;
        }
    };
} // namespace RenderCore
