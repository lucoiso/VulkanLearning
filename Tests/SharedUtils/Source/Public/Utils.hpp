// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#pragma once

#include <string>
#include <functional>
#include "RenderCoreTestsSharedUtilsModule.hpp"

import RenderCore.UserInterface.Window;
import RenderCore.UserInterface.Window.Flags;
import RenderCore.Utils.EnumHelpers;

class RENDERCORETESTSSHAREDUTILSMODULE_API ScopedTestWindow final
{
    RenderCore::Window m_Window {};

    std::string const                     m_WindowTitle { "Vulkan Renderer: Tests" };
    std::uint16_t const                   m_WindowWidth { 600U };
    std::uint16_t const                   m_WindowHeight { 600U };
    RenderCore::InitializationFlags const m_WindowFlags { RenderCore::InitializationFlags::HEADLESS };

public:
    ScopedTestWindow();
    ~ScopedTestWindow();

    RenderCore::Window &GetWindow();
    void                PollLoop(std::uint8_t);
    void                PollLoop(std::function<bool()> const &);
};
