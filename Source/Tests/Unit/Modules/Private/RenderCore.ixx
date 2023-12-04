// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <catch2/catch_test_macros.hpp>

export module Unit.RenderCore;

import RenderCore.Tests.SharedUtils;

import RenderCore.Window;
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
        Window.GetWindow().Shutdown().Get();
        REQUIRE_FALSE(Window.GetWindow().IsInitialized());
        REQUIRE_FALSE(Window.GetWindow().IsOpen());
    }
}

TEST_CASE("Scene Management", "[RenderCore]")
{
    ScopedWindow Window;

    auto const& LoadedObjects = RenderCore::EngineCore::Get().GetObjects();
    REQUIRE(std::empty(LoadedObjects));

    std::string const ObjectPath {"Resources/Assets/Box/glTF/Box.gltf"};
    std::string const ObjectName {"Mesh_000"};

    REQUIRE_FALSE(std::empty(RenderCore::EngineCore::Get().LoadScene(ObjectPath)));
    REQUIRE(LoadedObjects[0]->GetPath() == ObjectPath);
    REQUIRE(LoadedObjects[0]->GetName() == ObjectName);

    SECTION("Unload Default Scene")
    {
        RenderCore::EngineCore::Get().UnloadAllScenes();
        REQUIRE(std::empty(LoadedObjects));
        REQUIRE(std::empty(RenderCore::EngineCore::Get().GetObjects()));
    }

    SECTION("Reload Scene")
    {
        REQUIRE_FALSE(std::empty(RenderCore::EngineCore::Get().LoadScene(ObjectPath)));
        REQUIRE(LoadedObjects[0]->GetPath() == ObjectPath);
        REQUIRE(LoadedObjects[0]->GetName() == ObjectName);
    }
}