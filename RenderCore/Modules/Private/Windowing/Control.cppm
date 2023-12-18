// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <boost/log/trivial.hpp>

module RenderCore.Window.Control;

using namespace RenderCore;

Control::Control(Control* const Parent)
    : m_Parent(Parent)
{
}

Control::~Control()
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
        ProcessPaint(m_Children);
    }
    PostPaint();

    ProcessPaint(m_IndependentChildren);
}