// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <RenderCoreModule.h>

export module RenderCore.Window;

export import <cstdint>;
export import <string_view>;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Window
    {
    public:
        Window();

        Window(Window const&)            = delete;
        Window& operator=(Window const&) = delete;

        virtual ~Window();

        bool Initialize(std::uint16_t, std::uint16_t, std::string_view);
        static void Shutdown();

        [[nodiscard]] static bool IsInitialized();
        [[nodiscard]] static bool IsOpen();

        void PollEvents() const;

    protected:
        virtual void CreateOverlay();

    private:
        void RequestRender();
    };
}// namespace RenderCore