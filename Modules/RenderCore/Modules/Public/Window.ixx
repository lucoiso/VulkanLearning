// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <RenderCoreModule.h>

export module RenderCore.Window;

export import <cstdint>;
export import <string_view>;
export import <memory>;

export import RenderCore.Types.Coroutine;
import Timer.Manager;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Window
    {
        std::unique_ptr<Timer::Manager> m_RenderTimerManager {};

    public:
        Window();

        Window(Window const&)            = delete;
        Window& operator=(Window const&) = delete;

        virtual ~Window();

        AsyncOperation<bool> Initialize(std::uint16_t, std::uint16_t, std::string_view, bool const bHeadless = false);
        AsyncTask Shutdown();

        [[nodiscard]] bool IsInitialized();
        [[nodiscard]] bool IsOpen();

        virtual void PollEvents();

    protected:
        virtual void CreateOverlay();

    private:
        void RequestRender();
    };
}// namespace RenderCore