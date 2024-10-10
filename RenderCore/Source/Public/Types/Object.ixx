// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Object;

import RenderCore.Types.Resource;
import RenderCore.Types.Mesh;

// TODO : Check rendering if contains a object without a mesh
// TODO : Add logic to update rendering resources after changing object mesh

namespace RenderCore
{
    export class RENDERCOREMODULE_API Object : public Resource
    {
        std::shared_ptr<Mesh> m_Mesh{ nullptr };

    public:
        Object()           = delete;
        ~Object() override = default;

        inline bool operator==(Object const &Rhs) const
        {
            return GetID() == Rhs.GetID() && GetName() == Rhs.GetName() && GetPath() == Rhs.GetPath();
        }

        Object(std::uint32_t, strzilla::string_view);
        Object(std::uint32_t, strzilla::string_view, strzilla::string_view);

        [[nodiscard]] inline std::shared_ptr<Mesh> const &GetMesh() const
        {
            return m_Mesh;
        }

        inline void SetMesh(std::shared_ptr<Mesh> const &Value)
        {
            m_Mesh = Value;
        }

        void Destroy() override;

        virtual void Tick(double)
        {
        }

        void DrawObject(VkCommandBuffer const &, VkPipelineLayout const &, std::uint32_t) const;
    };
} // namespace RenderCore
