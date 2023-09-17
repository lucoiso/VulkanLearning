// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include <QMainWindow>
#include <string_view>
#include <cstdint>
#include <memory>

namespace RenderCore
{
    class RENDERCOREMODULE_API Window : public QMainWindow
    {
        class Renderer;

    public:
        Window();

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        ~Window();

        virtual void CreateContent() = 0;

        bool Initialize(const std::uint16_t Width, const std::uint16_t Height, const std::string_view Title);
        void Shutdown();

        bool IsInitialized() const;
        bool IsOpen() const;
        bool ShouldClose() const;

        void PollEvents();

    private:
        std::unique_ptr<Renderer> m_Renderer;
    };
}
