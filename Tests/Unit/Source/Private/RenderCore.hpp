// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#pragma once

#include <Utils.hpp>
#include <catch2/catch_test_macros.hpp>

import RenderCore.UserInterface.Window;
import RenderCore.Renderer;

// FIXME: Emitting validation errors

TEST_CASE("Window Status", "[RenderCore]")
{
    SECTION("Initialize")
    {
        ScopedTestWindow Window;

        // The window is already initialized by the ScopedWindow constructor
        REQUIRE(Window.GetWindow().IsOpen());
        REQUIRE(RenderCore::Renderer::IsInitialized());
    }

    SECTION("Shutdown")
    {
        // Window is already closed by the ScopedWindow destructor
        REQUIRE_FALSE(RenderCore::Renderer::IsInitialized());
    }
}

TEST_CASE("Scene Management", "[RenderCore]")
{
    ScopedTestWindow Window;

    auto const &LoadedObjects = RenderCore::Renderer::GetObjects();
    REQUIRE(std::empty(LoadedObjects));

    std::string const ObjectPath { "Models/Box/glTF/Box.gltf" };
    std::string const ObjectName { "Models/Box/glTF/Box" };

    RenderCore::Renderer::RequestLoadObject(ObjectPath);
    Window.PollLoop([]
    {
        return RenderCore::Renderer::GetNumObjects() == 0U;
    });

    REQUIRE(LoadedObjects[0]->GetPath() == ObjectPath);
    REQUIRE(LoadedObjects[0]->GetName() == ObjectName);

    SECTION("Unload Default Scene")
    {
        RenderCore::Renderer::RequestClearScene();
        Window.PollLoop([]
        {
            return RenderCore::Renderer::GetNumObjects() > 0U;
        });

        REQUIRE(std::empty(LoadedObjects));
    }

    SECTION("Reload Scene")
    {
        RenderCore::Renderer::RequestLoadObject(ObjectPath);
        Window.PollLoop([]
        {
            return RenderCore::Renderer::GetNumObjects() == 0U;
        });

        REQUIRE_FALSE(std::empty(LoadedObjects));
        REQUIRE(LoadedObjects[0]->GetPath() == ObjectPath);
        REQUIRE(LoadedObjects[0]->GetName() == ObjectName);
    }
}
