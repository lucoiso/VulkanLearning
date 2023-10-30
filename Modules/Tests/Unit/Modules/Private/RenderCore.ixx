//
// Created by User on 30.10.2023.
//

module;

#include <catch2/catch_test_macros.hpp>

export module Unit.RenderCore;

import RenderCore.Window;

TEST_CASE("Initialize Window", "[RenderCore]")
{
    SECTION("Initialize")
    {
        RenderCore::Window Window;
        REQUIRE(Window.Initialize(600U, 600U, "Vulkan Renderer", true).Get());
        REQUIRE(Window.IsOpen());
        REQUIRE(Window.IsInitialized());
        Window.Shutdown().Get();
        REQUIRE(!Window.IsInitialized());
    }
}