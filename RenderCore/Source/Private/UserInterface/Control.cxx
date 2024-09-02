// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

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

void Control::RemoveChild(std::shared_ptr<Control> const &Child)
{
    std::erase(m_Children, Child);
}

void Control::DestroyChildren()
{
    while (!std::empty(m_Children))
    {
        m_Children.pop_back();
    }
}

Control *Control::GetParent() const
{
    return m_Parent;
}

std::vector<std::shared_ptr<Control>> const &Control::GetChildren() const
{
    return m_Children;
}

void Control::Initialize()
{
    OnInitialize();

    Process(m_Children,
            [](std::shared_ptr<Control> const &Iterator)
            {
                Iterator->OnInitialize();
            });
}

void Control::Update()
{
    PrePaint();
    Paint();
    PostPaint();

    Process(m_Children,
            [](std::shared_ptr<Control> const &Iterator)
            {
                Iterator->PrePaint();
                Iterator->Paint();
                Iterator->PostPaint();
            });
}

void Control::RefreshResources()
{
    Refresh();

    Process(m_Children,
            [](std::shared_ptr<Control> const &Iterator)
            {
                Iterator->Refresh();
            });
}

void Control::PreUpdate()
{
    PreRender();

    Process(m_Children,
            [](std::shared_ptr<Control> const &Iterator)
            {
                Iterator->PreRender();
            });
}

void Control::PostUpdate()
{
    PostRender();

    Process(m_Children,
            [](std::shared_ptr<Control> const &Iterator)
            {
                Iterator->PostRender();
            });
}
