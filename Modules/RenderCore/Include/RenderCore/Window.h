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

        bool Initialize(const std::uint16_t Width, const std::uint16_t Height, const std::string_view Title);
        void Shutdown();

        bool IsInitialized() const;

        virtual void CreateWidgets();

    protected:
        void DrawFrame();
        virtual bool event(QEvent *const Event) override;

    private:
        std::unique_ptr<Renderer> m_Renderer;
        bool m_CanDraw;
    };
}
