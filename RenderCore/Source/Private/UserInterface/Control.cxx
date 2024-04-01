// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <vector>

module RenderCore.UserInterface.Control;

using namespace RenderCore;

Control::Control(Control *const Parent)
    : m_Parent(Parent)
{
}

Control::~Control()
{
    DestroyChildren();
}

void Control::DestroyChildren()
{
    while (!std::empty(m_Children))
    {
        m_Children.pop_back();
    }

    while (!std::empty(m_IndependentChildren))
    {
        m_IndependentChildren.pop_back();
    }
}

Control* Control::GetParent() const
{
    return m_Parent;
}

std::vector<std::shared_ptr<Control>> const& Control::GetChildren() const
{
    return m_Children;
}

std::vector<std::shared_ptr<Control>> const& Control::GetIndependentChildren() const
{
    return m_IndependentChildren;
}

void Control::Update()
{
    PrePaint();
    {
        Paint();

        Process(m_Children, &Control::PrePaint);
        Process(m_Children, &Control::Paint);
        Process(m_Children, &Control::PostPaint);
    }
    PostPaint();

    Process(m_IndependentChildren, &Control::PrePaint);
    Process(m_IndependentChildren, &Control::Paint);
    Process(m_IndependentChildren, &Control::PostPaint);
}

void Control::RefreshResources()
{
    Refresh();
    Process(m_Children, &Control::Refresh);
    Process(m_IndependentChildren, &Control::Refresh);
}

void Control::PreUpdate()
{
    RemoveInvalid(m_Children);
    RemoveInvalid(m_IndependentChildren);

    PreRender();
    Process(m_Children, &Control::PreRender);
    Process(m_IndependentChildren, &Control::PreRender);
}

void Control::PostUpdate()
{
    PostRender();
    Process(m_Children, &Control::PostRender);
    Process(m_IndependentChildren, &Control::PostRender);
}
