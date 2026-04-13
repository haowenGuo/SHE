#include "SHE/Platform/SdlWindowService.hpp"

#include "SHE/Core/Logger.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace she
{
namespace
{
std::size_t ToIndex(const KeyCode key)
{
    return static_cast<std::size_t>(key);
}

std::size_t ToIndex(const PointerButton button)
{
    return static_cast<std::size_t>(button);
}

void RequireMainThread(const char* operation)
{
    if (!SDL_IsMainThread())
    {
        throw std::runtime_error(std::string{"SDL window service must run on the main thread during "} + operation + ".");
    }
}

bool IsForActiveWindow(const WindowState& windowState, const SDL_WindowID eventWindowId)
{
    return eventWindowId == 0 || eventWindowId == windowState.windowId;
}

KeyCode TranslateScancode(const SDL_Scancode scancode)
{
    switch (scancode)
    {
    case SDL_SCANCODE_A:
        return KeyCode::A;
    case SDL_SCANCODE_B:
        return KeyCode::B;
    case SDL_SCANCODE_C:
        return KeyCode::C;
    case SDL_SCANCODE_D:
        return KeyCode::D;
    case SDL_SCANCODE_E:
        return KeyCode::E;
    case SDL_SCANCODE_F:
        return KeyCode::F;
    case SDL_SCANCODE_G:
        return KeyCode::G;
    case SDL_SCANCODE_H:
        return KeyCode::H;
    case SDL_SCANCODE_I:
        return KeyCode::I;
    case SDL_SCANCODE_J:
        return KeyCode::J;
    case SDL_SCANCODE_K:
        return KeyCode::K;
    case SDL_SCANCODE_L:
        return KeyCode::L;
    case SDL_SCANCODE_M:
        return KeyCode::M;
    case SDL_SCANCODE_N:
        return KeyCode::N;
    case SDL_SCANCODE_O:
        return KeyCode::O;
    case SDL_SCANCODE_P:
        return KeyCode::P;
    case SDL_SCANCODE_Q:
        return KeyCode::Q;
    case SDL_SCANCODE_R:
        return KeyCode::R;
    case SDL_SCANCODE_S:
        return KeyCode::S;
    case SDL_SCANCODE_T:
        return KeyCode::T;
    case SDL_SCANCODE_U:
        return KeyCode::U;
    case SDL_SCANCODE_V:
        return KeyCode::V;
    case SDL_SCANCODE_W:
        return KeyCode::W;
    case SDL_SCANCODE_X:
        return KeyCode::X;
    case SDL_SCANCODE_Y:
        return KeyCode::Y;
    case SDL_SCANCODE_Z:
        return KeyCode::Z;
    case SDL_SCANCODE_0:
        return KeyCode::Digit0;
    case SDL_SCANCODE_1:
        return KeyCode::Digit1;
    case SDL_SCANCODE_2:
        return KeyCode::Digit2;
    case SDL_SCANCODE_3:
        return KeyCode::Digit3;
    case SDL_SCANCODE_4:
        return KeyCode::Digit4;
    case SDL_SCANCODE_5:
        return KeyCode::Digit5;
    case SDL_SCANCODE_6:
        return KeyCode::Digit6;
    case SDL_SCANCODE_7:
        return KeyCode::Digit7;
    case SDL_SCANCODE_8:
        return KeyCode::Digit8;
    case SDL_SCANCODE_9:
        return KeyCode::Digit9;
    case SDL_SCANCODE_ESCAPE:
        return KeyCode::Escape;
    case SDL_SCANCODE_RETURN:
        return KeyCode::Enter;
    case SDL_SCANCODE_SPACE:
        return KeyCode::Space;
    case SDL_SCANCODE_TAB:
        return KeyCode::Tab;
    case SDL_SCANCODE_BACKSPACE:
        return KeyCode::Backspace;
    case SDL_SCANCODE_UP:
        return KeyCode::Up;
    case SDL_SCANCODE_DOWN:
        return KeyCode::Down;
    case SDL_SCANCODE_LEFT:
        return KeyCode::Left;
    case SDL_SCANCODE_RIGHT:
        return KeyCode::Right;
    case SDL_SCANCODE_LSHIFT:
        return KeyCode::ShiftLeft;
    case SDL_SCANCODE_RSHIFT:
        return KeyCode::ShiftRight;
    case SDL_SCANCODE_LCTRL:
        return KeyCode::ControlLeft;
    case SDL_SCANCODE_RCTRL:
        return KeyCode::ControlRight;
    case SDL_SCANCODE_LALT:
        return KeyCode::AltLeft;
    case SDL_SCANCODE_RALT:
        return KeyCode::AltRight;
    default:
        return KeyCode::Unknown;
    }
}

PointerButton TranslateMouseButton(const Uint8 button)
{
    switch (button)
    {
    case SDL_BUTTON_LEFT:
        return PointerButton::Left;
    case SDL_BUTTON_MIDDLE:
        return PointerButton::Middle;
    case SDL_BUTTON_RIGHT:
        return PointerButton::Right;
    case SDL_BUTTON_X1:
        return PointerButton::X1;
    case SDL_BUTTON_X2:
        return PointerButton::X2;
    default:
        return PointerButton::Count;
    }
}
} // namespace

void SdlWindowService::Initialize(const ApplicationConfig& config)
{
    RequireMainThread("Initialize");

    if (m_sdlInitialized)
    {
        throw std::runtime_error("SDL window service was initialized more than once.");
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error(std::string{"SDL_Init failed: "} + SDL_GetError());
    }

    m_sdlInitialized = true;
    m_title = config.applicationName;
    m_shouldClose = false;
    m_pumpCount = 0;
    m_pointerState = {};
    m_windowState = {};
    m_windowState.width = config.windowWidth;
    m_windowState.height = config.windowHeight;
    m_windowState.minimized = config.startWindowHidden;

    SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE;
    if (config.startWindowHidden)
    {
        windowFlags |= SDL_WINDOW_HIDDEN;
    }

    m_window = SDL_CreateWindow(m_title.c_str(), config.windowWidth, config.windowHeight, windowFlags);
    if (m_window == nullptr)
    {
        const std::string error = SDL_GetError();
        Shutdown();
        throw std::runtime_error("SDL_CreateWindow failed: " + error);
    }

    m_windowState.windowId = SDL_GetWindowID(m_window);
    UpdateWindowStateFromSdl();

    SHE_LOG_INFO("Platform", "SDL3 window initialized.");
}

void SdlWindowService::PumpEvents()
{
    RequireMainThread("PumpEvents");

    if (!m_sdlInitialized)
    {
        throw std::runtime_error("SDL window service cannot pump events before Initialize.");
    }

    ResetFrameSnapshot();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            m_shouldClose = true;
            m_windowState.closeRequested = true;
            break;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (IsForActiveWindow(m_windowState, event.window.windowID))
            {
                m_shouldClose = true;
                m_windowState.closeRequested = true;
            }
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            if (IsForActiveWindow(m_windowState, event.window.windowID))
            {
                m_windowState.width = event.window.data1;
                m_windowState.height = event.window.data2;
                m_windowState.resizedThisFrame = true;
            }
            break;

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        case SDL_EVENT_WINDOW_MINIMIZED:
        case SDL_EVENT_WINDOW_RESTORED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
            if (IsForActiveWindow(m_windowState, event.window.windowID))
            {
                UpdateWindowStateFromSdl();
            }
            break;

        case SDL_EVENT_KEY_DOWN:
            if (IsForActiveWindow(m_windowState, event.key.windowID))
            {
                const KeyCode key = TranslateScancode(event.key.scancode);
                if (key != KeyCode::Unknown)
                {
                    ButtonState& state = m_keyStates[ToIndex(key)];
                    if (!state.isDown && !event.key.repeat)
                    {
                        state.pressed = true;
                    }
                    state.isDown = true;
                }
            }
            break;

        case SDL_EVENT_KEY_UP:
            if (IsForActiveWindow(m_windowState, event.key.windowID))
            {
                const KeyCode key = TranslateScancode(event.key.scancode);
                if (key != KeyCode::Unknown)
                {
                    ButtonState& state = m_keyStates[ToIndex(key)];
                    if (state.isDown)
                    {
                        state.released = true;
                    }
                    state.isDown = false;
                }
            }
            break;

        case SDL_EVENT_MOUSE_MOTION:
            if (IsForActiveWindow(m_windowState, event.motion.windowID))
            {
                m_pointerState.x = static_cast<int>(event.motion.x);
                m_pointerState.y = static_cast<int>(event.motion.y);
                m_pointerState.deltaX += static_cast<int>(event.motion.xrel);
                m_pointerState.deltaY += static_cast<int>(event.motion.yrel);
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (IsForActiveWindow(m_windowState, event.button.windowID))
            {
                const PointerButton button = TranslateMouseButton(event.button.button);
                if (button != PointerButton::Count)
                {
                    ButtonState& state = m_pointerButtonStates[ToIndex(button)];
                    if (!state.isDown)
                    {
                        state.pressed = true;
                    }
                    state.isDown = true;
                }
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (IsForActiveWindow(m_windowState, event.button.windowID))
            {
                const PointerButton button = TranslateMouseButton(event.button.button);
                if (button != PointerButton::Count)
                {
                    ButtonState& state = m_pointerButtonStates[ToIndex(button)];
                    if (state.isDown)
                    {
                        state.released = true;
                    }
                    state.isDown = false;
                }
            }
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            if (IsForActiveWindow(m_windowState, event.wheel.windowID))
            {
                float wheelX = event.wheel.x;
                float wheelY = event.wheel.y;
                if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
                {
                    wheelX = -wheelX;
                    wheelY = -wheelY;
                }

                m_pointerState.wheelX += wheelX;
                m_pointerState.wheelY += wheelY;
            }
            break;

        default:
            break;
        }
    }

    ++m_pumpCount;
    UpdateWindowStateFromSdl();
}

bool SdlWindowService::ShouldClose() const
{
    return m_shouldClose;
}

void SdlWindowService::RequestClose()
{
    m_shouldClose = true;
    m_windowState.closeRequested = true;
}

ButtonState SdlWindowService::GetKeyState(const KeyCode key) const
{
    if (key == KeyCode::Unknown || key == KeyCode::Count)
    {
        return {};
    }

    return m_keyStates[ToIndex(key)];
}

ButtonState SdlWindowService::GetPointerButtonState(const PointerButton button) const
{
    if (button == PointerButton::Count)
    {
        return {};
    }

    return m_pointerButtonStates[ToIndex(button)];
}

PointerState SdlWindowService::GetPointerState() const
{
    return m_pointerState;
}

WindowState SdlWindowService::GetWindowState() const
{
    return m_windowState;
}

std::size_t SdlWindowService::GetPumpCount() const
{
    return m_pumpCount;
}

void SdlWindowService::Shutdown()
{
    if (m_window != nullptr)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    if (m_sdlInitialized)
    {
        SDL_Quit();
        m_sdlInitialized = false;
    }

    m_title.clear();
    m_shouldClose = false;
    m_pumpCount = 0;
    m_keyStates = {};
    m_pointerButtonStates = {};
    m_pointerState = {};
    m_windowState = {};

    SHE_LOG_INFO("Platform", "SDL3 window shutdown complete.");
}

void SdlWindowService::ResetFrameSnapshot()
{
    for (ButtonState& state : m_keyStates)
    {
        state.pressed = false;
        state.released = false;
    }

    for (ButtonState& state : m_pointerButtonStates)
    {
        state.pressed = false;
        state.released = false;
    }

    m_pointerState.deltaX = 0;
    m_pointerState.deltaY = 0;
    m_pointerState.wheelX = 0.0F;
    m_pointerState.wheelY = 0.0F;
    m_windowState.resizedThisFrame = false;
}

void SdlWindowService::UpdateWindowStateFromSdl()
{
    if (m_window == nullptr)
    {
        return;
    }

    m_windowState.windowId = SDL_GetWindowID(m_window);

    if (!m_windowState.resizedThisFrame)
    {
        int width = 0;
        int height = 0;
        SDL_GetWindowSize(m_window, &width, &height);
        m_windowState.width = width;
        m_windowState.height = height;
    }

    const SDL_WindowFlags flags = SDL_GetWindowFlags(m_window);
    m_windowState.hasKeyboardFocus = (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
    m_windowState.hasMouseFocus = (flags & SDL_WINDOW_MOUSE_FOCUS) != 0;
    m_windowState.minimized = (flags & SDL_WINDOW_MINIMIZED) != 0;
}
} // namespace she
