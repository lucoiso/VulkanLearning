// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include "RenderCoreModule.h"

export module RenderCoreWindow;

import <thread>;
import <mutex>;
import <queue>;
import <string_view>;
import <stdexcept>;
import <unordered_map>;

export namespace RenderCore
{
    class RENDERCOREMODULE_API Window
    {
        class Impl;
        std::unique_ptr<Impl> m_Impl;

    public:
        Window();

        Window(Window const&)            = delete;
        Window& operator=(Window const&) = delete;

        virtual ~Window();

        bool Initialize(std::uint16_t Width, std::uint16_t Height, std::string_view Title);
        void Shutdown() const;

        [[nodiscard]] bool IsInitialized() const;
        [[nodiscard]] bool IsOpen() const;

        void PollEvents() const;

        virtual void CreateOverlay();
    };
}// namespace RenderCore