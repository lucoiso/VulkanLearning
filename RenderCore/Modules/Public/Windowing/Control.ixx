// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RenderCoreModule.hpp"
#include <memory>
#include <vector>

export module RenderCore.Window.Control;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Control
    {
        Control* m_Parent {nullptr};
        std::vector<std::shared_ptr<Control>> m_Children {};
        std::vector<std::shared_ptr<Control>> m_IndependentChildren {};

    protected:
        explicit Control(Control* Parent);

    public:
        Control()                          = delete;
        Control& operator=(Control const&) = delete;

        virtual ~Control();

        template<typename ControlTy, typename... Args>
        void AddChild(Args&&... Arguments)
        {
            m_Children.emplace_back(std::make_shared<ControlTy>(this, std::forward<Args>(Arguments)...));
        }

        template<typename ControlTy, typename... Args>
        void AddIndependentChild(Args&&... Arguments)
        {
            m_IndependentChildren.emplace_back(std::make_shared<ControlTy>(this, std::forward<Args>(Arguments)...));
        }

        [[nodiscard]] Control* GetParent() const;
        [[nodiscard]] std::vector<std::shared_ptr<Control>> const& GetChildren() const;
        [[nodiscard]] std::vector<std::shared_ptr<Control>> const& GetIndependentChildren() const;

        void Update();

    protected:
        void RefreshResources();
        void PreUpdate();
        void PostUpdate();

        virtual void Refresh()
        {
        }
        virtual void PreRender()
        {
        }
        virtual void PrePaint()
        {
        }
        virtual void Paint()
        {
        }
        virtual void PostPaint()
        {
        }
        virtual void PostRender()
        {
        }

    private:
        static constexpr void RemoveInvalid(std::vector<std::shared_ptr<Control>>& Children)
        {
            std::erase_if(Children, [](std::shared_ptr<Control> const& Child) {
                return !Child;
            });
        }

        template<typename Functor>
        static constexpr void Process(std::vector<std::shared_ptr<Control>>& Children, Functor&& Call)
        {
            RemoveInvalid(Children);

            for (auto const& Child: Children)
            {
                if (Child)
                {
                    (Child.get()->*Call)();
                }
            }
        }
    };
}// namespace RenderCore