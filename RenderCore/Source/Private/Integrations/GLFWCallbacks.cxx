// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Integrations.GLFWCallbacks;

import RenderCore.Renderer;
import RenderCore.Runtime.Device;
import RenderCore.Types.Camera;
import RenderCore.Types.Transform;
import RenderCore.Types.RendererStateFlags;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumHelpers;
import RenderCore.UserInterface.Window.Flags;
import RenderCore.Integrations.ImGuiOverlay;
import RenderCore.Integrations.ImGuiGLFWBackend;

using namespace RenderCore;

bool g_CanMovementCamera = false;
bool g_CanMovementWindow = false;

void RenderCore::GLFWWindowCloseRequestedCallback(GLFWwindow *const Window)
{
    std::lock_guard const Lock { GetRendererMutex() };
    glfwSetWindowShouldClose(Window, GLFW_TRUE);
}

void RenderCore::GLFWWindowResizedCallback([[maybe_unused]] GLFWwindow *const  Window,
                                           [[maybe_unused]] std::int32_t const Width,
                                           [[maybe_unused]] std::int32_t const Height)
{
    std::lock_guard const Lock { GetRendererMutex() };

    constexpr RendererStateFlags RefreshFlags = RendererStateFlags::INVALID_SIZE | RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE;

    if (Width <= 0 || Height <= 0)
    {
        Renderer::AddStateFlag(RefreshFlags);
    }
    else
    {
        Renderer::RemoveStateFlag(RefreshFlags);
    }

    Renderer::RequestUpdateResources();
}

void RenderCore::GLFWErrorCallback(std::int32_t const Error, char const *const Description)
{
    BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: GLFW Error: " << Error << " - " << Description;
}

void RenderCore::GLFWKeyCallback([[maybe_unused]] GLFWwindow *const  Window,
                                 std::int32_t const                  Key,
                                 [[maybe_unused]] std::int32_t const Scancode,
                                 std::int32_t const                  Action,
                                 [[maybe_unused]] std::int32_t const Mods)
{
    Camera &                 Camera               = Renderer::GetMutableCamera();
    CameraMovementStateFlags CurrentMovementState = Camera.GetCameraMovementStateFlags();

    if (!g_CanMovementCamera)
    {
        Camera.SetCameraMovementStateFlags(CameraMovementStateFlags::NONE);
        return;
    }

    if (Action == GLFW_PRESS)
    {
        switch (Key)
        {
            case GLFW_KEY_W:
                AddFlags(CurrentMovementState, CameraMovementStateFlags::FORWARD);
                break;
            case GLFW_KEY_S:
                AddFlags(CurrentMovementState, CameraMovementStateFlags::BACKWARD);
                break;
            case GLFW_KEY_A:
                AddFlags(CurrentMovementState, CameraMovementStateFlags::LEFT);
                break;
            case GLFW_KEY_D:
                AddFlags(CurrentMovementState, CameraMovementStateFlags::RIGHT);
                break;
            case GLFW_KEY_Q:
                AddFlags(CurrentMovementState, CameraMovementStateFlags::DOWN);
                break;
            case GLFW_KEY_E:
            case GLFW_KEY_SPACE:
                AddFlags(CurrentMovementState, CameraMovementStateFlags::UP);
                break;
            default:
                break;
        }
    }
    else if (Action == GLFW_RELEASE)
    {
        switch (Key)
        {
            case GLFW_KEY_W:
                RemoveFlags(CurrentMovementState, CameraMovementStateFlags::FORWARD);
                break;
            case GLFW_KEY_S:
                RemoveFlags(CurrentMovementState, CameraMovementStateFlags::BACKWARD);
                break;
            case GLFW_KEY_A:
                RemoveFlags(CurrentMovementState, CameraMovementStateFlags::LEFT);
                break;
            case GLFW_KEY_D:
                RemoveFlags(CurrentMovementState, CameraMovementStateFlags::RIGHT);
                break;
            case GLFW_KEY_Q:
                RemoveFlags(CurrentMovementState, CameraMovementStateFlags::DOWN);
                break;
            case GLFW_KEY_E:
            case GLFW_KEY_SPACE:
                RemoveFlags(CurrentMovementState, CameraMovementStateFlags::UP);
                break;
            default:
                break;
        }
    }

    Camera.SetCameraMovementStateFlags(CurrentMovementState);
}

static void MovementWindow(GLFWwindow *const Window, double const NewCursorPosX, double const NewCursorPosY)
{
    if (!g_CanMovementWindow)
    {
        return;
    }

    static double InitialCursorPosX = 0.0;
    static double InitialCursorPosY = 0.0;
    static bool   IsDragging        = false;

    if (HasFlag(RenderCore::Renderer::GetWindowInitializationFlags(), InitializationFlags::WITHOUT_TITLEBAR))
    {
        if (glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            if (!IsDragging)
            {
                glfwGetCursorPos(Window, &InitialCursorPosX, &InitialCursorPosY);
                IsDragging = true;
            }

            std::int32_t WindowX;
            std::int32_t WindowY;
            glfwGetWindowPos(Window, &WindowX, &WindowY);

            auto const NewPosX = WindowX + static_cast<std::int32_t>(NewCursorPosX - InitialCursorPosX);
            auto const NewPosY = WindowY + static_cast<std::int32_t>(NewCursorPosY - InitialCursorPosY);

            glfwSetWindowPos(Window, NewPosX, NewPosY);
        }
        else
        {
            IsDragging = false;
        }
    }
}

static void MovementCamera(GLFWwindow *const Window, double const NewCursorPosX, double const NewCursorPosY)
{
    if (RenderCore::IsImGuiInitialized() && ImGui::IsAnyItemHovered())
    {
        return;
    }

    static double LastCursorPosX = NewCursorPosX;
    static double LastCursorPosY = NewCursorPosY;

    float const OffsetX { static_cast<float>(LastCursorPosX - NewCursorPosX) };
    float const OffsetY { static_cast<float>(LastCursorPosY - NewCursorPosY) };

    if (g_CanMovementCamera)
    {
        glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        Camera &Camera { Renderer::GetMutableCamera() };

        float const Sensitivity { Camera.GetSensitivity() * 0.1F };

        glm::vec3 Rotation { Camera.GetRotation() };

        Rotation.x -= OffsetX * Sensitivity;
        Rotation.y += OffsetY * Sensitivity;

        if (Rotation.y > 89.F)
        {
            Rotation.y = 89.F;
        }
        else if (Rotation.y < -89.F)
        {
            Rotation.y = -89.F;
        }

        Camera.SetRotation(Rotation);
    }
    else
    {
        glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    LastCursorPosX = NewCursorPosX;
    LastCursorPosY = NewCursorPosY;
}

void RenderCore::GLFWCursorPositionCallback(GLFWwindow *const Window, double const NewCursorPosX, double const NewCursorPosY)
{
    if (IsImGuiInitialized())
    {
        ImGuiGLFWUpdateMouse();
    }

    static bool HasReleasedLeft = true;
    if (glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !g_CanMovementWindow && HasReleasedLeft)
    {
        g_CanMovementWindow = RenderCore::IsImGuiInitialized() && !ImGui::IsAnyItemHovered();
        HasReleasedLeft     = false;
    }
    else if (glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        g_CanMovementWindow = false;
        HasReleasedLeft     = true;
    }

    g_CanMovementCamera = glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_RELEASE;

    MovementWindow(Window, NewCursorPosX, NewCursorPosY);
    MovementCamera(Window, NewCursorPosX, NewCursorPosY);
}

void RenderCore::GLFWCursorScrollCallback([[maybe_unused]] GLFWwindow *const Window, [[maybe_unused]] double const OffsetX, double const OffsetY)
{
    if (RenderCore::IsImGuiInitialized() && ImGui::IsAnyItemHovered())
    {
        return;
    }

    Camera &    Camera = Renderer::GetMutableCamera();
    float const Zoom   = static_cast<float>(OffsetY) * 0.1f;
    Camera.SetPosition(Camera.GetPosition() + Camera.GetFront() * Zoom);
}

void RenderCore::InstallGLFWCallbacks(GLFWwindow *const Window, bool const InstallClose)
{
    if (InstallClose)
    {
        glfwSetWindowCloseCallback(Window, &GLFWWindowCloseRequestedCallback);
    }

    glfwSetWindowSizeCallback(Window, &GLFWWindowResizedCallback);
    glfwSetKeyCallback(Window, &GLFWKeyCallback);
    glfwSetCursorPosCallback(Window, &GLFWCursorPositionCallback);
    glfwSetScrollCallback(Window, &GLFWCursorScrollCallback);
}
