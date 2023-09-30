// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include <string_view>
#include <cstdint>
#include <memory>

namespace RenderCore
{
    class RENDERCOREMODULE_API Window
    {
        class Impl;

    public:
        Window();

        Window(const Window&)            = delete;
        Window& operator=(const Window&) = delete;

        ~Window();

        bool Initialize(std::uint16_t Width, std::uint16_t Height, std::string_view Title);
        void Shutdown() const;

        bool IsInitialized() const;
        bool IsOpen() const;

        void PollEvents() const;

        virtual void CreateOverlay()
        {
        };

    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
