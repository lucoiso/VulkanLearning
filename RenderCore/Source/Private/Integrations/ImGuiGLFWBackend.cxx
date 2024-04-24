// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

// Adapted from: https://github.com/ocornut/imgui/blob/docking/backends/imgui_impl_glfw.cpp

module;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
#include <array>
#include <cstdint>
#include <imgui.h>
#include <vector>
#include <Volk/volk.h>

// GLFW after Volk
#include <GLFW/glfw3.h>

#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#ifdef __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#endif
#endif

module RenderCore.Integrations.ImGuiGLFWBackend;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
using namespace RenderCore;

struct ImGuiGLFWData
{
    GLFWwindow *                                     Window {};
    double                                           Time {};
    GLFWwindow *                                     MouseWindow {};
    std::array<GLFWcursor *, ImGuiMouseCursor_COUNT> MouseCursors {};
    ImVec2                                           LastValidMousePos;
    std::array<GLFWwindow *, GLFW_KEY_LAST>          KeyOwnerWindows {};
    bool                                             InstalledCallbacks {};
    bool                                             CallbacksChainForAllWindows {};
    bool                                             WantUpdateMonitors {};

    GLFWwindowfocusfun PrevUserCallbackWindowFocus {};
    GLFWcursorposfun   PrevUserCallbackCursorPos {};
    GLFWcursorenterfun PrevUserCallbackCursorEnter {};
    GLFWmousebuttonfun PrevUserCallbackMousebutton {};
    GLFWscrollfun      PrevUserCallbackScroll {};
    GLFWkeyfun         PrevUserCallbackKey {};
    GLFWcharfun        PrevUserCallbackChar {};
    GLFWmonitorfun     PrevUserCallbackMonitor {};

#ifdef _WIN32
    WNDPROC PrevWndProc {};
#endif

    ImGuiGLFWData()
    {
        memset(this, 0U, sizeof(*this));
    }
};

struct ImGuiGLFWViewportData
{
    GLFWwindow * Window {};
    bool         WindowOwned {};
    std::int32_t IgnoreWindowPosEventFrame {};
    std::int32_t IgnoreWindowSizeEventFrame {};
#ifdef _WIN32
    WNDPROC PrevWndProc {};
#endif

    ImGuiGLFWViewportData()
    {
        memset(this, 0U, sizeof(*this));
        IgnoreWindowSizeEventFrame = IgnoreWindowPosEventFrame = -1;
    }

    ~ImGuiGLFWViewportData() = default;
};

ImGuiGLFWData *ImGuiGLFWGetBackendData()
{
    return ImGui::GetCurrentContext() ? static_cast<ImGuiGLFWData *>(ImGui::GetIO().BackendPlatformUserData) : nullptr;
}

const char *ImGuiGLFWGetClipboardText(void *UserData)
{
    return glfwGetClipboardString(static_cast<GLFWwindow *>(UserData));
}

void ImGuiGLFWSetClipboardText(void *UserData, const char *Text)
{
    glfwSetClipboardString(static_cast<GLFWwindow *>(UserData), Text);
}

ImGuiKey ImGuiGLFWKeyToImGuiKey(std::int32_t const Key)
{
    switch (Key)
    {
        case GLFW_KEY_TAB:
            return ImGuiKey_Tab;
        case GLFW_KEY_LEFT:
            return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT:
            return ImGuiKey_RightArrow;
        case GLFW_KEY_UP:
            return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN:
            return ImGuiKey_DownArrow;
        case GLFW_KEY_PAGE_UP:
            return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN:
            return ImGuiKey_PageDown;
        case GLFW_KEY_HOME:
            return ImGuiKey_Home;
        case GLFW_KEY_END:
            return ImGuiKey_End;
        case GLFW_KEY_INSERT:
            return ImGuiKey_Insert;
        case GLFW_KEY_DELETE:
            return ImGuiKey_Delete;
        case GLFW_KEY_BACKSPACE:
            return ImGuiKey_Backspace;
        case GLFW_KEY_SPACE:
            return ImGuiKey_Space;
        case GLFW_KEY_ENTER:
            return ImGuiKey_Enter;
        case GLFW_KEY_ESCAPE:
            return ImGuiKey_Escape;
        case GLFW_KEY_APOSTROPHE:
            return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA:
            return ImGuiKey_Comma;
        case GLFW_KEY_MINUS:
            return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD:
            return ImGuiKey_Period;
        case GLFW_KEY_SLASH:
            return ImGuiKey_Slash;
        case GLFW_KEY_SEMICOLON:
            return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL:
            return ImGuiKey_Equal;
        case GLFW_KEY_LEFT_BRACKET:
            return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH:
            return ImGuiKey_Backslash;
        case GLFW_KEY_RIGHT_BRACKET:
            return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT:
            return ImGuiKey_GraveAccent;
        case GLFW_KEY_CAPS_LOCK:
            return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK:
            return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK:
            return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN:
            return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE:
            return ImGuiKey_Pause;
        case GLFW_KEY_KP_0:
            return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1:
            return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2:
            return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3:
            return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4:
            return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5:
            return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6:
            return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7:
            return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8:
            return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9:
            return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL:
            return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE:
            return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY:
            return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT:
            return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD:
            return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER:
            return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL:
            return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT:
            return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL:
            return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT:
            return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER:
            return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT:
            return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL:
            return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT:
            return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER:
            return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU:
            return ImGuiKey_Menu;
        case GLFW_KEY_0:
            return ImGuiKey_0;
        case GLFW_KEY_1:
            return ImGuiKey_1;
        case GLFW_KEY_2:
            return ImGuiKey_2;
        case GLFW_KEY_3:
            return ImGuiKey_3;
        case GLFW_KEY_4:
            return ImGuiKey_4;
        case GLFW_KEY_5:
            return ImGuiKey_5;
        case GLFW_KEY_6:
            return ImGuiKey_6;
        case GLFW_KEY_7:
            return ImGuiKey_7;
        case GLFW_KEY_8:
            return ImGuiKey_8;
        case GLFW_KEY_9:
            return ImGuiKey_9;
        case GLFW_KEY_A:
            return ImGuiKey_A;
        case GLFW_KEY_B:
            return ImGuiKey_B;
        case GLFW_KEY_C:
            return ImGuiKey_C;
        case GLFW_KEY_D:
            return ImGuiKey_D;
        case GLFW_KEY_E:
            return ImGuiKey_E;
        case GLFW_KEY_F:
            return ImGuiKey_F;
        case GLFW_KEY_G:
            return ImGuiKey_G;
        case GLFW_KEY_H:
            return ImGuiKey_H;
        case GLFW_KEY_I:
            return ImGuiKey_I;
        case GLFW_KEY_J:
            return ImGuiKey_J;
        case GLFW_KEY_K:
            return ImGuiKey_K;
        case GLFW_KEY_L:
            return ImGuiKey_L;
        case GLFW_KEY_M:
            return ImGuiKey_M;
        case GLFW_KEY_N:
            return ImGuiKey_N;
        case GLFW_KEY_O:
            return ImGuiKey_O;
        case GLFW_KEY_P:
            return ImGuiKey_P;
        case GLFW_KEY_Q:
            return ImGuiKey_Q;
        case GLFW_KEY_R:
            return ImGuiKey_R;
        case GLFW_KEY_S:
            return ImGuiKey_S;
        case GLFW_KEY_T:
            return ImGuiKey_T;
        case GLFW_KEY_U:
            return ImGuiKey_U;
        case GLFW_KEY_V:
            return ImGuiKey_V;
        case GLFW_KEY_W:
            return ImGuiKey_W;
        case GLFW_KEY_X:
            return ImGuiKey_X;
        case GLFW_KEY_Y:
            return ImGuiKey_Y;
        case GLFW_KEY_Z:
            return ImGuiKey_Z;
        case GLFW_KEY_F1:
            return ImGuiKey_F1;
        case GLFW_KEY_F2:
            return ImGuiKey_F2;
        case GLFW_KEY_F3:
            return ImGuiKey_F3;
        case GLFW_KEY_F4:
            return ImGuiKey_F4;
        case GLFW_KEY_F5:
            return ImGuiKey_F5;
        case GLFW_KEY_F6:
            return ImGuiKey_F6;
        case GLFW_KEY_F7:
            return ImGuiKey_F7;
        case GLFW_KEY_F8:
            return ImGuiKey_F8;
        case GLFW_KEY_F9:
            return ImGuiKey_F9;
        case GLFW_KEY_F10:
            return ImGuiKey_F10;
        case GLFW_KEY_F11:
            return ImGuiKey_F11;
        case GLFW_KEY_F12:
            return ImGuiKey_F12;
        case GLFW_KEY_F13:
            return ImGuiKey_F13;
        case GLFW_KEY_F14:
            return ImGuiKey_F14;
        case GLFW_KEY_F15:
            return ImGuiKey_F15;
        case GLFW_KEY_F16:
            return ImGuiKey_F16;
        case GLFW_KEY_F17:
            return ImGuiKey_F17;
        case GLFW_KEY_F18:
            return ImGuiKey_F18;
        case GLFW_KEY_F19:
            return ImGuiKey_F19;
        case GLFW_KEY_F20:
            return ImGuiKey_F20;
        case GLFW_KEY_F21:
            return ImGuiKey_F21;
        case GLFW_KEY_F22:
            return ImGuiKey_F22;
        case GLFW_KEY_F23:
            return ImGuiKey_F23;
        case GLFW_KEY_F24:
            return ImGuiKey_F24;
        default:
            return ImGuiKey_None;
    }
}

void ImGuiGLFWUpdateKeyModifiers(GLFWwindow *Window)
{
    ImGuiIO &ImGuiIO = ImGui::GetIO();

    ImGuiIO.AddKeyEvent(ImGuiMod_Ctrl,
                        glfwGetKey(Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(Window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);

    ImGuiIO.AddKeyEvent(ImGuiMod_Shift,
                        glfwGetKey(Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(Window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    ImGuiIO.AddKeyEvent(ImGuiMod_Alt, glfwGetKey(Window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(Window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);

    ImGuiIO.AddKeyEvent(ImGuiMod_Super,
                        glfwGetKey(Window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS || glfwGetKey(Window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS);
}

bool ImGuiGLFWShouldChainCallback(GLFWwindow const *Window)
{
    ImGuiGLFWData const *BackendData = ImGuiGLFWGetBackendData();
    return BackendData->CallbacksChainForAllWindows || Window == BackendData->Window;
}

void RenderCore::ImGuiGLFWMouseButtonCallback(GLFWwindow *       Window,
                                              std::int32_t const Button,
                                              std::int32_t const Action,
                                              std::int32_t const Modifications)
{
    if (ImGuiGLFWData const *Backend = ImGuiGLFWGetBackendData();
        Backend->PrevUserCallbackMousebutton && ImGuiGLFWShouldChainCallback(Window))
    {
        Backend->PrevUserCallbackMousebutton(Window, Button, Action, Modifications);
    }

    ImGuiGLFWUpdateKeyModifiers(Window);

    if (Button >= 0 && Button < ImGuiMouseButton_COUNT)
    {
        ImGui::GetIO().AddMouseButtonEvent(Button, Action == GLFW_PRESS);
    }
}

void RenderCore::ImGuiGLFWScrollCallback(GLFWwindow *Window, double const OffsetX, double const OffsetY)
{
    if (ImGuiGLFWData const *Backend = ImGuiGLFWGetBackendData();
        Backend->PrevUserCallbackScroll && ImGuiGLFWShouldChainCallback(Window))
    {
        Backend->PrevUserCallbackScroll(Window, OffsetX, OffsetY);
    }

    ImGui::GetIO().AddMouseWheelEvent(static_cast<float>(OffsetX), static_cast<float>(OffsetY));
}

void RenderCore::ImGuiGLFWKeyCallback(GLFWwindow *       Window,
                                      std::int32_t const KeyCode,
                                      std::int32_t const ScanCode,
                                      std::int32_t const Action,
                                      std::int32_t const Modifications)
{
    ImGuiGLFWData *Backend = ImGuiGLFWGetBackendData();
    if (Backend->PrevUserCallbackKey != nullptr && ImGuiGLFWShouldChainCallback(Window))
    {
        Backend->PrevUserCallbackKey(Window, KeyCode, ScanCode, Action, Modifications);
    }

    if (Action != GLFW_PRESS && Action != GLFW_RELEASE)
    {
        return;
    }

    ImGuiGLFWUpdateKeyModifiers(Window);

    if (KeyCode >= 0 && KeyCode < std::size(Backend->KeyOwnerWindows))
    {
        Backend->KeyOwnerWindows.at(KeyCode) = Action == GLFW_PRESS ? Window : nullptr;
    }

    ImGuiIO &      ImGuiIO = ImGui::GetIO();
    ImGuiKey const Key     = ImGuiGLFWKeyToImGuiKey(KeyCode);

    ImGuiIO.AddKeyEvent(Key, Action == GLFW_PRESS);
    ImGuiIO.SetKeyEventNativeData(Key, KeyCode, ScanCode);
}

void RenderCore::ImGuiGLFWWindowFocusCallback(GLFWwindow *Window, std::int32_t const Focus)
{
    if (ImGuiGLFWData const *Backend = ImGuiGLFWGetBackendData();
        Backend->PrevUserCallbackWindowFocus != nullptr && ImGuiGLFWShouldChainCallback(Window))
    {
        Backend->PrevUserCallbackWindowFocus(Window, Focus);
    }

    ImGui::GetIO().AddFocusEvent(Focus != 0);
}

void RenderCore::ImGuiGLFWCursorPosCallback(GLFWwindow *Window, double PosX, double PosY)
{
    ImGuiGLFWData *Backend = ImGuiGLFWGetBackendData();
    if (Backend->PrevUserCallbackCursorPos != nullptr && ImGuiGLFWShouldChainCallback(Window))
    {
        Backend->PrevUserCallbackCursorPos(Window, PosX, PosY);
    }

    ImGuiIO &ImGuiIO = ImGui::GetIO();
    if (ImGuiIO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        std::int32_t WindowX;
        std::int32_t WindowY;
        glfwGetWindowPos(Window, &WindowX, &WindowY);
        PosX += WindowX;
        PosY += WindowY;
    }

    ImGuiIO.AddMousePosEvent(static_cast<float>(PosX), static_cast<float>(PosY));
    Backend->LastValidMousePos = ImVec2(static_cast<float>(PosX), static_cast<float>(PosY));
}

void RenderCore::ImGuiGLFWCursorEnterCallback(GLFWwindow *Window, std::int32_t const Entered)
{
    ImGuiGLFWData *Backend = ImGuiGLFWGetBackendData();
    if (Backend->PrevUserCallbackCursorEnter != nullptr && ImGuiGLFWShouldChainCallback(Window))
    {
        Backend->PrevUserCallbackCursorEnter(Window, Entered);
    }

    ImGuiIO &ImGuiIO = ImGui::GetIO();

    if (Entered)
    {
        Backend->MouseWindow = Window;
        ImGuiIO.AddMousePosEvent(Backend->LastValidMousePos.x, Backend->LastValidMousePos.y);
    }
    else
    {
        Backend->LastValidMousePos = ImGuiIO.MousePos;
        Backend->MouseWindow       = nullptr;
        ImGuiIO.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }
}

void RenderCore::ImGuiGLFWCharCallback(GLFWwindow *Window, std::uint32_t const Character)
{
    if (ImGuiGLFWData const *Backend = ImGuiGLFWGetBackendData();
        Backend->PrevUserCallbackChar != nullptr && ImGuiGLFWShouldChainCallback(Window))
    {
        Backend->PrevUserCallbackChar(Window, Character);
    }

    ImGuiIO &ImGuiIO = ImGui::GetIO();
    ImGuiIO.AddInputCharacter(Character);
}

void RenderCore::ImGuiGLFWMonitorCallback([[maybe_unused]] GLFWmonitor *const Monitor, [[maybe_unused]] std::int32_t const Index)
{
    ImGuiGLFWData *Backend      = ImGuiGLFWGetBackendData();
    Backend->WantUpdateMonitors = true;
}

void RenderCore::ImGuiGLFWInstallCallbacks(GLFWwindow *Window)
{
    ImGuiGLFWData *Backend = ImGuiGLFWGetBackendData();

    Backend->PrevUserCallbackWindowFocus = glfwSetWindowFocusCallback(Window, ImGuiGLFWWindowFocusCallback);
    Backend->PrevUserCallbackCursorEnter = glfwSetCursorEnterCallback(Window, ImGuiGLFWCursorEnterCallback);
    Backend->PrevUserCallbackCursorPos   = glfwSetCursorPosCallback(Window, ImGuiGLFWCursorPosCallback);
    Backend->PrevUserCallbackMousebutton = glfwSetMouseButtonCallback(Window, ImGuiGLFWMouseButtonCallback);
    Backend->PrevUserCallbackScroll      = glfwSetScrollCallback(Window, ImGuiGLFWScrollCallback);
    Backend->PrevUserCallbackKey         = glfwSetKeyCallback(Window, ImGuiGLFWKeyCallback);
    Backend->PrevUserCallbackChar        = glfwSetCharCallback(Window, ImGuiGLFWCharCallback);
    Backend->PrevUserCallbackMonitor     = glfwSetMonitorCallback(ImGuiGLFWMonitorCallback);
    Backend->InstalledCallbacks          = true;
}

void RenderCore::ImGuiGLFWRestoreCallbacks(GLFWwindow *Window)
{
    ImGuiGLFWData *Backend = ImGuiGLFWGetBackendData();

    glfwSetWindowFocusCallback(Window, Backend->PrevUserCallbackWindowFocus);
    glfwSetCursorEnterCallback(Window, Backend->PrevUserCallbackCursorEnter);
    glfwSetCursorPosCallback(Window, Backend->PrevUserCallbackCursorPos);
    glfwSetMouseButtonCallback(Window, Backend->PrevUserCallbackMousebutton);
    glfwSetScrollCallback(Window, Backend->PrevUserCallbackScroll);
    glfwSetKeyCallback(Window, Backend->PrevUserCallbackKey);
    glfwSetCharCallback(Window, Backend->PrevUserCallbackChar);
    glfwSetMonitorCallback(Backend->PrevUserCallbackMonitor);

    Backend->InstalledCallbacks          = false;
    Backend->PrevUserCallbackWindowFocus = nullptr;
    Backend->PrevUserCallbackCursorEnter = nullptr;
    Backend->PrevUserCallbackCursorPos   = nullptr;
    Backend->PrevUserCallbackMousebutton = nullptr;
    Backend->PrevUserCallbackScroll      = nullptr;
    Backend->PrevUserCallbackKey         = nullptr;
    Backend->PrevUserCallbackChar        = nullptr;
    Backend->PrevUserCallbackMonitor     = nullptr;
}

void RenderCore::ImGuiGLFWSetCallbacksChainForAllWindows(bool const Chain)
{
    ImGuiGLFWData *Backend               = ImGuiGLFWGetBackendData();
    Backend->CallbacksChainForAllWindows = Chain;
}

#ifdef _WIN32
LRESULT CALLBACK ImGuiGLFWWndProc(HWND const Handle, UINT const Message, WPARAM const WParam, LPARAM const LParam)
{
    ImGuiGLFWData const *Backend           = ImGuiGLFWGetBackendData();
    WNDPROC              PreviousProcedure = Backend->PrevWndProc;

    if (ImGuiViewport const *Viewport = static_cast<ImGuiViewport *>(GetPropA(Handle, "IMGUI_VIEWPORT"));
        Viewport != nullptr)
    {
        if (ImGuiGLFWViewportData const *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData))
        {
            PreviousProcedure = ViewportData->PrevWndProc;
        }
    }

    switch (Message)
    {
        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        case WM_XBUTTONUP:
            ImGui::GetIO().AddMouseSourceEvent(ImGuiMouseSource_Mouse);
            break;
        default:
            break;
    }
    return CallWindowProcW(PreviousProcedure, Handle, Message, WParam, LParam);
}
#endif

bool ImGuiGLFWInit(GLFWwindow *Window, bool const InstallCallbacks)
{
    ImGuiIO &ImGuiIO = ImGui::GetIO();

    auto *Backend                   = IM_NEW(ImGuiGLFWData)();
    ImGuiIO.BackendPlatformUserData = static_cast<void *>(Backend);
    ImGuiIO.BackendPlatformName     = "RenderCore_ImGui_GLFW";
    ImGuiIO.BackendFlags |= ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos | ImGuiBackendFlags_PlatformHasViewports |
            ImGuiBackendFlags_HasMouseHoveredViewport;

    Backend->Window             = Window;
    Backend->Time               = 0.0;
    Backend->WantUpdateMonitors = true;

    ImGuiIO.SetClipboardTextFn = ImGuiGLFWSetClipboardText;
    ImGuiIO.GetClipboardTextFn = ImGuiGLFWGetClipboardText;
    ImGuiIO.ClipboardUserData  = Backend->Window;

    GLFWerrorfun const ErrorCallback                      = glfwSetErrorCallback(nullptr);
    Backend->MouseCursors.at(ImGuiMouseCursor_Arrow)      = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    Backend->MouseCursors.at(ImGuiMouseCursor_TextInput)  = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    Backend->MouseCursors.at(ImGuiMouseCursor_ResizeNS)   = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    Backend->MouseCursors.at(ImGuiMouseCursor_ResizeEW)   = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    Backend->MouseCursors.at(ImGuiMouseCursor_Hand)       = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    Backend->MouseCursors.at(ImGuiMouseCursor_ResizeAll)  = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    Backend->MouseCursors.at(ImGuiMouseCursor_ResizeNESW) = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    Backend->MouseCursors.at(ImGuiMouseCursor_ResizeNWSE) = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    Backend->MouseCursors.at(ImGuiMouseCursor_NotAllowed) = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);

    glfwSetErrorCallback(ErrorCallback);
    [[maybe_unused]] auto const _ = glfwGetError(nullptr);

    if (InstallCallbacks)
    {
        ImGuiGLFWInstallCallbacks(Window);
    }

    ImGuiGLFWUpdateMonitors();
    glfwSetMonitorCallback(ImGuiGLFWMonitorCallback);

    ImGuiViewport *MainViewport  = ImGui::GetMainViewport();
    MainViewport->PlatformHandle = static_cast<void *>(Backend->Window);
#ifdef _WIN32
    MainViewport->PlatformHandleRaw = glfwGetWin32Window(Backend->Window);
#elif defined(__APPLE__)
    MainViewport->PlatformHandleRaw = static_cast<void *>(glfwGetCocoaWindow(Backend->Window));
#else
    IM_UNUSED(MainViewport);
#endif

    if (ImGuiIO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGuiGLFWInitPlatformInterface();
    }

#ifdef _WIN32
    Backend->PrevWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(static_cast<HWND>(MainViewport->PlatformHandleRaw), GWLP_WNDPROC));
    IM_ASSERT(Backend->PrevWndProc != nullptr);
    SetWindowLongPtrW(static_cast<HWND>(MainViewport->PlatformHandleRaw), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ImGuiGLFWWndProc));
#endif

    return true;
}

bool RenderCore::ImGuiGLFWInitForVulkan(GLFWwindow *Window, bool const InstallCallbacks)
{
    return ImGuiGLFWInit(Window, InstallCallbacks);
}

void RenderCore::ImGuiGLFWShutdown()
{
    ImGuiGLFWData *Backend = ImGuiGLFWGetBackendData();
    ImGuiIO &      ImGuiIO = ImGui::GetIO();

    ImGuiGLFWShutdownPlatformInterface();

    if (Backend->InstalledCallbacks)
    {
        ImGuiGLFWRestoreCallbacks(Backend->Window);
    }

    for (std::uint8_t CursorIt = 0U; CursorIt < ImGuiMouseCursor_COUNT; ++CursorIt)
    {
        glfwDestroyCursor(Backend->MouseCursors.at(CursorIt));
    }

#ifdef _WIN32
    ImGuiViewport const *MainViewport = ImGui::GetMainViewport();
    SetWindowLongPtrW(static_cast<HWND>(MainViewport->PlatformHandleRaw), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Backend->PrevWndProc));
    Backend->PrevWndProc = nullptr;
#endif

    ImGuiIO.BackendPlatformName     = nullptr;
    ImGuiIO.BackendPlatformUserData = nullptr;
    ImGuiIO.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos | ImGuiBackendFlags_PlatformHasViewports |
                              ImGuiBackendFlags_HasMouseHoveredViewport);

    IM_DELETE(Backend);
}

void ImGuiGLFWUpdateMouseData()
{
    ImGuiGLFWData *  Backend    = ImGuiGLFWGetBackendData();
    ImGuiIO &        ImGuiIO    = ImGui::GetIO();
    ImGuiPlatformIO &PlatformIO = ImGui::GetPlatformIO();

    ImGuiID      MouseViewportId = 0U;
    const ImVec2 MousePosPrev    = ImGuiIO.MousePos;

    for (std::uint32_t ViewportSizeIt = 0U; ViewportSizeIt < PlatformIO.Viewports.Size; ++ViewportSizeIt)
    {
        ImGuiViewport const *Viewport = PlatformIO.Viewports[static_cast<std::int32_t>(ViewportSizeIt)];
        auto *               Window   = static_cast<GLFWwindow *>(Viewport->PlatformHandle);

        if (glfwGetWindowAttrib(Window, GLFW_FOCUSED))
        {
            if (ImGuiIO.WantSetMousePos)
            {
                glfwSetCursorPos(Window, MousePosPrev.x - Viewport->Pos.x, MousePosPrev.y - Viewport->Pos.y);
            }

            if (Backend->MouseWindow == nullptr)
            {
                double MouseX = 0.0;
                double MouseY = 0.0;

                glfwGetCursorPos(Window, &MouseX, &MouseY);

                if (ImGuiIO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                {
                    std::int32_t WindowX;
                    std::int32_t WindowY;

                    glfwGetWindowPos(Window, &WindowX, &WindowY);

                    MouseX += WindowX;
                    MouseY += WindowY;
                }

                Backend->LastValidMousePos = ImVec2(static_cast<float>(MouseX), static_cast<float>(MouseY));
                ImGuiIO.AddMousePosEvent(static_cast<float>(MouseX), static_cast<float>(MouseY));
            }
        }

        const bool HasNoInputFlag = Viewport->Flags & ImGuiViewportFlags_NoInputs;
        glfwSetWindowAttrib(Window, GLFW_MOUSE_PASSTHROUGH, HasNoInputFlag);

        if (glfwGetWindowAttrib(Window, GLFW_HOVERED))
        {
            MouseViewportId = Viewport->ID;
        }
    }

    if (ImGuiIO.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
    {
        ImGuiIO.AddMouseViewportEvent(MouseViewportId);
    }
}

void ImGuiGLFWUpdateMouseCursor()
{
    ImGuiIO const &      ImGuiIO = ImGui::GetIO();
    ImGuiGLFWData const *Backend = ImGuiGLFWGetBackendData();
    if (ImGuiIO.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange || glfwGetInputMode(Backend->Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
    {
        return;
    }

    ImGuiMouseCursor const ImGuiCursor = ImGui::GetMouseCursor();
    ImGuiPlatformIO &      PlatformIO  = ImGui::GetPlatformIO();
    for (std::uint32_t ViewportIt = 0U; ViewportIt < PlatformIO.Viewports.Size; ++ViewportIt)
    {
        auto *Window = static_cast<GLFWwindow *>(PlatformIO.Viewports[static_cast<std::int32_t>(ViewportIt)]->PlatformHandle);

        if (ImGuiCursor == ImGuiMouseCursor_None || ImGuiIO.MouseDrawCursor)
        {
            glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
        else
        {
            glfwSetCursor(Window,
                          Backend->MouseCursors.at(ImGuiCursor)
                              ? Backend->MouseCursors.at(ImGuiCursor)
                              : Backend->MouseCursors.at(ImGuiMouseCursor_Arrow));

            glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void RenderCore::ImGuiGLFWUpdateMonitors()
{
    ImGuiGLFWData *  Backend    = ImGuiGLFWGetBackendData();
    ImGuiPlatformIO &PlatformIO = ImGui::GetPlatformIO();
    Backend->WantUpdateMonitors = false;

    std::int32_t  NumMonitors = 0;
    GLFWmonitor **Monitors    = glfwGetMonitors(&NumMonitors);

    if (NumMonitors == 0)
    {
        return;
    }

    PlatformIO.Monitors.resize(0);

    for (std::uint32_t MonitorIt = 0U; MonitorIt < NumMonitors; ++MonitorIt)
    {
        ImGuiPlatformMonitor PlatformMonitor;
        std::int32_t         PosX;
        std::int32_t         PosY;
        glfwGetMonitorPos(Monitors[MonitorIt], &PosX, &PosY);

        const GLFWvidmode *VideoMode = glfwGetVideoMode(Monitors[MonitorIt]);
        if (VideoMode == nullptr)
        {
            continue;
        }

        PlatformMonitor.MainPos  = PlatformMonitor.WorkPos  = ImVec2(static_cast<float>(PosX), static_cast<float>(PosY));
        PlatformMonitor.MainSize = PlatformMonitor.WorkSize = ImVec2(static_cast<float>(VideoMode->width), static_cast<float>(VideoMode->height));

        std::int32_t Width;
        std::int32_t Height;
        glfwGetMonitorWorkarea(Monitors[MonitorIt], &PosX, &PosY, &Width, &Height);

        if (Width > 0 && Height > 0)
        {
            PlatformMonitor.WorkPos  = ImVec2(static_cast<float>(PosX), static_cast<float>(PosY));
            PlatformMonitor.WorkSize = ImVec2(static_cast<float>(Width), static_cast<float>(Height));
        }

        float ScaleX;
        float ScaleY;
        glfwGetMonitorContentScale(Monitors[MonitorIt], &ScaleX, &ScaleY);

        PlatformMonitor.DpiScale       = ScaleX;
        PlatformMonitor.PlatformHandle = static_cast<void *>(Monitors[MonitorIt]);

        PlatformIO.Monitors.push_back(PlatformMonitor);
    }
}

void RenderCore::ImGuiGLFWNewFrame()
{
    ImGuiIO &      ImGuiIO = ImGui::GetIO();
    ImGuiGLFWData *Backend = ImGuiGLFWGetBackendData();

    std::int32_t Width;
    std::int32_t Height;
    std::int32_t DisplayWidth;
    std::int32_t DisplayHeight;

    glfwGetWindowSize(Backend->Window, &Width, &Height);
    glfwGetFramebufferSize(Backend->Window, &DisplayWidth, &DisplayHeight);

    ImGuiIO.DisplaySize = ImVec2(static_cast<float>(Width), static_cast<float>(Height));

    if (Width > 0 && Height > 0)
    {
        ImGuiIO.DisplayFramebufferScale = ImVec2(static_cast<float>(DisplayWidth) / static_cast<float>(Width),
                                                 static_cast<float>(DisplayHeight) / static_cast<float>(Height));
    }

    if (Backend->WantUpdateMonitors)
    {
        ImGuiGLFWUpdateMonitors();
    }

    double CurrentTime = glfwGetTime();

    if (CurrentTime <= Backend->Time)
    {
        CurrentTime = Backend->Time + 0.00001F;
    }

    ImGuiIO.DeltaTime = Backend->Time > 0.0 ? static_cast<float>(CurrentTime - Backend->Time) : 1.F / 60.F;
    Backend->Time     = CurrentTime;

    ImGuiGLFWUpdateMouseData();
    ImGuiGLFWUpdateMouseCursor();
}

void ImGuiGLFWWindowCloseCallback(GLFWwindow *Window)
{
    if (ImGuiViewport *Viewport = ImGui::FindViewportByPlatformHandle(Window))
    {
        Viewport->PlatformRequestClose = true;
    }
}

void ImGuiGLFWWindowPosCallback(GLFWwindow *Window, std::int32_t, std::int32_t)
{
    if (ImGuiViewport *Viewport = ImGui::FindViewportByPlatformHandle(Window))
    {
        if (ImGuiGLFWViewportData const *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData))
        {
            if (ImGui::GetFrameCount() <= ViewportData->IgnoreWindowPosEventFrame + 1)
            {
                return;
            }
        }

        Viewport->PlatformRequestMove = true;
    }
}

void ImGuiGLFWWindowSizeCallback(GLFWwindow *Window, std::int32_t, std::int32_t)
{
    if (ImGuiViewport *Viewport = ImGui::FindViewportByPlatformHandle(Window))
    {
        if (ImGuiGLFWViewportData const *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData))
        {
            if (ImGui::GetFrameCount() <= ViewportData->IgnoreWindowSizeEventFrame + 1)
            {
                return;
            }
        }

        Viewport->PlatformRequestResize = true;
    }
}

void ImGuiGLFWCreateWindow(ImGuiViewport *Viewport)
{
    auto *ViewportData         = IM_NEW(ImGuiGLFWViewportData)();
    Viewport->PlatformUserData = ViewportData;

    glfwWindowHint(GLFW_VISIBLE, false);
    glfwWindowHint(GLFW_FOCUSED, false);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, false);
    glfwWindowHint(GLFW_DECORATED, Viewport->Flags & ImGuiViewportFlags_NoDecoration ? false : true);
    glfwWindowHint(GLFW_FLOATING, Viewport->Flags & ImGuiViewportFlags_TopMost ? true : false);

    ViewportData->Window      = glfwCreateWindow(static_cast<int>(Viewport->Size.x), static_cast<int>(Viewport->Size.y), "Undef", nullptr, nullptr);
    ViewportData->WindowOwned = true;
    Viewport->PlatformHandle  = static_cast<void *>(ViewportData->Window);

#ifdef _WIN32
    Viewport->PlatformHandleRaw = glfwGetWin32Window(ViewportData->Window);
#elif defined(__APPLE__)
    Viewport->PlatformHandleRaw = static_cast<void *>(glfwGetCocoaWindow(ViewportData->Window));
#endif

    glfwSetWindowPos(ViewportData->Window, static_cast<int>(Viewport->Pos.x), static_cast<int>(Viewport->Pos.y));

    glfwSetWindowFocusCallback(ViewportData->Window, ImGuiGLFWWindowFocusCallback);
    glfwSetCursorEnterCallback(ViewportData->Window, ImGuiGLFWCursorEnterCallback);
    glfwSetCursorPosCallback(ViewportData->Window, ImGuiGLFWCursorPosCallback);
    glfwSetMouseButtonCallback(ViewportData->Window, ImGuiGLFWMouseButtonCallback);
    glfwSetScrollCallback(ViewportData->Window, ImGuiGLFWScrollCallback);
    glfwSetKeyCallback(ViewportData->Window, ImGuiGLFWKeyCallback);
    glfwSetCharCallback(ViewportData->Window, ImGuiGLFWCharCallback);
    glfwSetWindowCloseCallback(ViewportData->Window, ImGuiGLFWWindowCloseCallback);
    glfwSetWindowPosCallback(ViewportData->Window, ImGuiGLFWWindowPosCallback);
    glfwSetWindowSizeCallback(ViewportData->Window, ImGuiGLFWWindowSizeCallback);
}

void ImGuiGLFWDestroyWindow(ImGuiViewport *Viewport)
{
    ImGuiGLFWData const *Backend = ImGuiGLFWGetBackendData();

    if (auto *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData))
    {
        if (ViewportData->WindowOwned)
        {
            for (std::uint32_t KeyIt = 0U; KeyIt < std::size(Backend->KeyOwnerWindows); ++KeyIt)
            {
                if (Backend->KeyOwnerWindows.at(KeyIt) == ViewportData->Window)
                {
                    ImGuiGLFWKeyCallback(ViewportData->Window, KeyIt, 0, GLFW_RELEASE, 0);
                }
            }

            glfwDestroyWindow(ViewportData->Window);
        }

        ViewportData->Window = nullptr;
        IM_DELETE(ViewportData);
    }

    Viewport->PlatformUserData = nullptr;
    Viewport->PlatformHandle   = nullptr;
}

void ImGuiGLFWShowWindow(ImGuiViewport *Viewport)
{
    ImGuiGLFWViewportData const *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData);

#ifdef _WIN32
    if (Viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon)
    {
        auto const Handle = static_cast<HWND>(Viewport->PlatformHandleRaw);
        LONG       Style  = ::GetWindowLong(Handle, GWL_EXSTYLE);
        Style &= ~WS_EX_APPWINDOW;
        Style |= WS_EX_TOOLWINDOW;
        ::SetWindowLong(Handle, GWL_EXSTYLE, Style);
    }
#endif

    glfwShowWindow(ViewportData->Window);
}

ImVec2 ImGuiGLFWGetWindowPos(ImGuiViewport *Viewport)
{
    ImGuiGLFWViewportData const *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData);
    std::int32_t                 PosX         = 0;
    std::int32_t                 PosY         = 0;
    glfwGetWindowPos(ViewportData->Window, &PosX, &PosY);

    return { static_cast<float>(PosX), static_cast<float>(PosY) };
}

void ImGuiGLFWSetWindowPos(ImGuiViewport *Viewport, ImVec2 const Position)
{
    auto *ViewportData                      = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData);
    ViewportData->IgnoreWindowPosEventFrame = ImGui::GetFrameCount();
    glfwSetWindowPos(ViewportData->Window, static_cast<int>(Position.x), static_cast<int>(Position.y));
}

ImVec2 ImGuiGLFWGetWindowSize(ImGuiViewport *Viewport)
{
    ImGuiGLFWViewportData const *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData);
    std::int32_t                 Width        = 0;
    std::int32_t                 Height       = 0;
    glfwGetWindowSize(ViewportData->Window, &Width, &Height);
    return { static_cast<float>(Width), static_cast<float>(Height) };
}

void ImGuiGLFWSetWindowSize(ImGuiViewport *Viewport, ImVec2 const Size)
{
    auto *ViewportData                       = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData);
    ViewportData->IgnoreWindowSizeEventFrame = ImGui::GetFrameCount();
    glfwSetWindowSize(ViewportData->Window, static_cast<int>(Size.x), static_cast<int>(Size.y));
}

void ImGuiGLFWSetWindowTitle(ImGuiViewport *Viewport, const char *Title)
{
    ImGuiGLFWViewportData const *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData);
    glfwSetWindowTitle(ViewportData->Window, Title);
}

void ImGuiGLFWSetWindowFocus(ImGuiViewport *Viewport)
{
    ImGuiGLFWViewportData const *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData);
    glfwFocusWindow(ViewportData->Window);
}

bool ImGuiGLFWGetWindowFocus(ImGuiViewport *Viewport)
{
    ImGuiGLFWViewportData const *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData);
    return glfwGetWindowAttrib(ViewportData->Window, GLFW_FOCUSED);
}

bool ImGuiGLFWGetWindowMinimized(ImGuiViewport *Viewport)
{
    ImGuiGLFWViewportData const *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData);
    return glfwGetWindowAttrib(ViewportData->Window, GLFW_ICONIFIED);
}

void ImGuiGLFWSetWindowAlpha(ImGuiViewport *Viewport, float const Alpha)
{
    ImGuiGLFWViewportData const *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData);
    glfwSetWindowOpacity(ViewportData->Window, Alpha);
}

std::int32_t ImGuiGLFWCreateVkSurface(ImGuiViewport *Viewport, ImU64 const Instance, const void *Allocator, ImU64 *Surface)
{
    ImGuiGLFWViewportData const *ViewportData = static_cast<ImGuiGLFWViewportData *>(Viewport->PlatformUserData);
    VkResult const               Result       = glfwCreateWindowSurface(reinterpret_cast<VkInstance>(Instance),
                                                                        ViewportData->Window,
                                                                        static_cast<const VkAllocationCallbacks *>(Allocator),
                                                                        reinterpret_cast<VkSurfaceKHR *>(Surface));
    return Result;
}

void RenderCore::ImGuiGLFWInitPlatformInterface()
{
    ImGuiGLFWData const *Backend           = ImGuiGLFWGetBackendData();
    ImGuiPlatformIO &    PlatformIO        = ImGui::GetPlatformIO();
    PlatformIO.Platform_CreateWindow       = ImGuiGLFWCreateWindow;
    PlatformIO.Platform_DestroyWindow      = ImGuiGLFWDestroyWindow;
    PlatformIO.Platform_ShowWindow         = ImGuiGLFWShowWindow;
    PlatformIO.Platform_SetWindowPos       = ImGuiGLFWSetWindowPos;
    PlatformIO.Platform_GetWindowPos       = ImGuiGLFWGetWindowPos;
    PlatformIO.Platform_SetWindowSize      = ImGuiGLFWSetWindowSize;
    PlatformIO.Platform_GetWindowSize      = ImGuiGLFWGetWindowSize;
    PlatformIO.Platform_SetWindowFocus     = ImGuiGLFWSetWindowFocus;
    PlatformIO.Platform_GetWindowFocus     = ImGuiGLFWGetWindowFocus;
    PlatformIO.Platform_GetWindowMinimized = ImGuiGLFWGetWindowMinimized;
    PlatformIO.Platform_SetWindowTitle     = ImGuiGLFWSetWindowTitle;
    PlatformIO.Platform_SetWindowAlpha     = ImGuiGLFWSetWindowAlpha;
    PlatformIO.Platform_CreateVkSurface    = ImGuiGLFWCreateVkSurface;

    ImGuiViewport *MainViewport    = ImGui::GetMainViewport();
    auto const     ViewportData    = IM_NEW(ImGuiGLFWViewportData)();
    ViewportData->Window           = Backend->Window;
    ViewportData->WindowOwned      = false;
    MainViewport->PlatformUserData = ViewportData;
    MainViewport->PlatformHandle   = static_cast<void *>(Backend->Window);
}

void RenderCore::ImGuiGLFWShutdownPlatformInterface()
{
    ImGui::DestroyPlatformWindows();
}
#endif
