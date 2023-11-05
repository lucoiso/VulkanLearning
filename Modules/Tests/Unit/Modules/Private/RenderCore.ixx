//
// Created by User on 30.10.2023.
//

module;

#include <catch2/catch_test_macros.hpp>

export module Unit.RenderCore;

import RenderCore.Tests.SharedUtils;

import RenderCore.Window;
import RenderCore.Utils.RenderUtils;

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

    auto const LoadedIDs = RenderCore::GetLoadedIDs();
    REQUIRE_FALSE(std::empty(LoadedIDs));

    auto const& LoadedObjects = RenderCore::GetLoadedObjects();
    REQUIRE_FALSE(std::empty(LoadedObjects));

    std::string const DefaultObjectPath(LoadedObjects[0].GetPath());
    REQUIRE(DefaultObjectPath == "Resources/Assets/VIKING_ROOM_OBJ.obj");
    REQUIRE_FALSE(std::empty(DefaultObjectPath));

    std::string const DefaultObjectName(LoadedObjects[0].GetName());
    REQUIRE(DefaultObjectName == "VIKING_ROOM_OBJ");
    REQUIRE_FALSE(std::empty(DefaultObjectName));

    SECTION("Unload Default Scene")
    {
        RenderCore::UnloadObject(LoadedIDs);
        REQUIRE(std::empty(RenderCore::GetLoadedIDs()));
        REQUIRE(std::empty(LoadedObjects));
    }

    SECTION("Load Default Scene")
    {
        auto const NewLoadedIDs = RenderCore::LoadObject(DefaultObjectPath, "");
        REQUIRE_FALSE(std::empty(NewLoadedIDs));
        REQUIRE_FALSE(std::empty(LoadedObjects));

        std::string const NewObjectPath(LoadedObjects[0].GetPath());
        REQUIRE_FALSE(std::empty(NewObjectPath));
        REQUIRE(NewObjectPath == "Resources/Assets/VIKING_ROOM_OBJ.obj");
        REQUIRE(NewObjectPath == DefaultObjectPath);

        std::string const NewObjectName(LoadedObjects[0].GetName());
        REQUIRE_FALSE(std::empty(NewObjectName));
        REQUIRE(NewObjectName == "VIKING_ROOM_OBJ");
        REQUIRE(NewObjectName == DefaultObjectName);
    }
}