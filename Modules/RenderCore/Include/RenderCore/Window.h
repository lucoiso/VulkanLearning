// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include <QtQuick/QQuickView>
#include <string_view>
#include <cstdint>
#include <memory>

class QEvent;

namespace RenderCore
{
    class RENDERCOREMODULE_API Window : public QQuickView
    {
        class Impl;

    public:
        Window();

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        ~Window();

        bool Initialize(const std::uint16_t Width, const std::uint16_t Height, const std::string_view Title);
        void Shutdown();

        bool IsInitialized() const;

        virtual void CreateOverlay();

    protected:
        void DrawFrame();
        virtual bool event(QEvent *const Event) override;

    private:
        std::unique_ptr<Impl> m_Impl;
        bool m_CanDraw;
    };
}
