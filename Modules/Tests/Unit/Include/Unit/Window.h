// Copyright Notices: [...]

#pragma once

#define BOOST_TEST_MODULE WINDOW_TEST_MODULE
#include <boost/test/included/unit_test.hpp>
#include <RenderCore/Window.h>

BOOST_AUTO_TEST_CASE(InsertData)
{
    std::unique_ptr<RenderCore::Window> Window = std::make_unique<RenderCore::Window>();
    BOOST_TEST(Window != nullptr);
    BOOST_TEST(Window->Initialize(800u, 600u, "Vulkan Project"));
    BOOST_TEST(Window->IsInitialized());
    BOOST_TEST(Window->IsOpen());
    BOOST_TEST(!Window->ShouldClose());

    Window->Shutdown();
    BOOST_TEST(!Window->IsInitialized());
    BOOST_TEST(!Window->IsOpen());
    BOOST_TEST(Window->ShouldClose());

    Window.reset();
    BOOST_TEST(Window == nullptr);
}