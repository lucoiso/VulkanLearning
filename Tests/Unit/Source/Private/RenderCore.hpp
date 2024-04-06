// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#pragma once

#include <Utils.hpp>
#include <catch2/catch_test_macros.hpp>

import RenderCore.UserInterface.Window;
import RenderCore.Renderer;

TEST_CASE("Window Status", "[RenderCore]")
{
    ScopedWindow Window;

    SECTION("Initialize")
    {
        // The window is already initialized by the ScopedWindow constructor
        REQUIRE(Window.GetWindow().IsOpen());
        REQUIRE(Window.GetWindow().IsInitialized());
    }

    SECTION("Shutdown")
    {
        Window.GetWindow().Shutdown();
        REQUIRE_FALSE(Window.GetWindow().IsInitialized());
        REQUIRE_FALSE(Window.GetWindow().IsOpen());
    }
}

TEST_CASE("Scene Management", "[RenderCore]")
{
    ScopedWindow Window;

    auto const& LoadedObjects = Window.GetWindow().GetRenderer().GetObjects();
    REQUIRE(std::empty(LoadedObjects));

    std::string const ObjectPath {"Resources/Assets/Box/glTF/Box.gltf"};
    std::string const ObjectName {"Mesh_000"};

    REQUIRE_FALSE(std::empty(Window.GetWindow().GetRenderer().LoadScene(ObjectPath)));
    REQUIRE(LoadedObjects[0]->GetPath() == ObjectPath);
    REQUIRE(LoadedObjects[0]->GetName() == ObjectName);

    SECTION("Unload Default Scene")
    {
        Window.GetWindow().GetRenderer().UnloadAllScenes();
        REQUIRE(std::empty(LoadedObjects));
        REQUIRE(std::empty(Window.GetWindow().GetRenderer().GetObjects()));
    }

    SECTION("Reload Scene")
    {
        REQUIRE_FALSE(std::empty(Window.GetWindow().GetRenderer().LoadScene(ObjectPath)));
        REQUIRE(LoadedObjects[0]->GetPath() == ObjectPath);
        REQUIRE(LoadedObjects[0]->GetName() == ObjectName);
    }
}