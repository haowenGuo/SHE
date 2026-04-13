#include <SDL3/SDL_main.h>

#include "SHE/Core/ApplicationConfig.hpp"
#include "SHE/Platform/SdlWindowService.hpp"

#include <SDL3/SDL.h>

#include <iostream>
#include <string_view>

namespace
{
bool Expect(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }

    return true;
}

bool PushEvent(SDL_Event event)
{
    if (!SDL_PushEvent(&event))
    {
        std::cerr << "SDL_PushEvent failed: " << SDL_GetError() << '\n';
        return false;
    }

    return true;
}

bool TestWindowLifecycleAndCloseSignal()
{
    she::SdlWindowService window;

    she::ApplicationConfig config;
    config.applicationName = "SHE Platform Lifecycle Test";
    config.windowWidth = 640;
    config.windowHeight = 360;
    config.startWindowHidden = true;

    window.Initialize(config);

    bool success = true;

    const she::WindowState initialState = window.GetWindowState();
    const she::NativeWindowHandle nativeHandle = window.GetNativeWindowHandle();
    success &= Expect(initialState.windowId != 0, "Expected SDL window initialization to produce a native window id.");
    success &= Expect(initialState.width == 640, "Expected SDL window width to match the application config.");
    success &= Expect(initialState.height == 360, "Expected SDL window height to match the application config.");
    success &= Expect(
        nativeHandle.backend == she::NativeWindowBackend::Sdl3 && nativeHandle.handle != nullptr,
        "Expected SDL window initialization to publish a valid SDL native window handle.");
    success &= Expect(window.GetPumpCount() == 0, "Expected no event pumps before the first platform tick.");
    success &= Expect(!window.ShouldClose(), "Expected the hidden SDL test window to start open.");

    window.PumpEvents();
    success &= Expect(window.GetPumpCount() == 1, "Expected PumpEvents to advance the platform tick counter.");

    SDL_Event closeEvent{};
    closeEvent.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    closeEvent.window.windowID = initialState.windowId;
    success &= PushEvent(closeEvent);

    window.PumpEvents();
    success &= Expect(window.ShouldClose(), "Expected a close-request event to mark the SDL window service for shutdown.");
    success &= Expect(window.GetWindowState().closeRequested, "Expected closeRequested to stay visible in the window snapshot.");

    window.Shutdown();
    return success;
}

bool TestInputSnapshotsTrackEdgesAndPointerData()
{
    she::SdlWindowService window;

    she::ApplicationConfig config;
    config.applicationName = "SHE Platform Input Test";
    config.startWindowHidden = true;

    window.Initialize(config);

    const she::WindowState initialState = window.GetWindowState();

    SDL_Event keyDown{};
    keyDown.type = SDL_EVENT_KEY_DOWN;
    keyDown.key.windowID = initialState.windowId;
    keyDown.key.scancode = SDL_SCANCODE_W;
    keyDown.key.repeat = false;

    SDL_Event mouseMotion{};
    mouseMotion.type = SDL_EVENT_MOUSE_MOTION;
    mouseMotion.motion.windowID = initialState.windowId;
    mouseMotion.motion.x = 200.0F;
    mouseMotion.motion.y = 120.0F;
    mouseMotion.motion.xrel = 7.0F;
    mouseMotion.motion.yrel = -5.0F;

    SDL_Event mouseDown{};
    mouseDown.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    mouseDown.button.windowID = initialState.windowId;
    mouseDown.button.button = SDL_BUTTON_LEFT;

    SDL_Event mouseWheel{};
    mouseWheel.type = SDL_EVENT_MOUSE_WHEEL;
    mouseWheel.wheel.windowID = initialState.windowId;
    mouseWheel.wheel.x = 1.0F;
    mouseWheel.wheel.y = -2.0F;
    mouseWheel.wheel.direction = SDL_MOUSEWHEEL_NORMAL;

    SDL_Event resize{};
    resize.type = SDL_EVENT_WINDOW_RESIZED;
    resize.window.windowID = initialState.windowId;
    resize.window.data1 = 800;
    resize.window.data2 = 600;

    bool success = true;
    success &= PushEvent(keyDown);
    success &= PushEvent(mouseMotion);
    success &= PushEvent(mouseDown);
    success &= PushEvent(mouseWheel);
    success &= PushEvent(resize);

    window.PumpEvents();

    const she::ButtonState keyState = window.GetKeyState(she::KeyCode::W);
    const she::ButtonState pointerButtonState = window.GetPointerButtonState(she::PointerButton::Left);
    const she::PointerState pointerState = window.GetPointerState();
    const she::WindowState windowState = window.GetWindowState();

    success &= Expect(keyState.isDown, "Expected W to stay held after a key-down event.");
    success &= Expect(keyState.pressed, "Expected W to register a pressed edge on the first pump.");
    success &= Expect(!keyState.released, "Expected W not to register a release edge during key down.");
    success &= Expect(pointerButtonState.isDown, "Expected the left mouse button to stay held after button down.");
    success &= Expect(pointerButtonState.pressed, "Expected the left mouse button to report a pressed edge.");
    success &= Expect(pointerState.x == 200 && pointerState.y == 120, "Expected pointer position to follow SDL mouse motion.");
    success &= Expect(
        pointerState.deltaX == 7 && pointerState.deltaY == -5,
        "Expected pointer delta to accumulate within the current frame snapshot.");
    success &= Expect(pointerState.wheelX == 1.0F && pointerState.wheelY == -2.0F, "Expected pointer wheel deltas to remain visible for one frame.");
    success &= Expect(windowState.width == 800 && windowState.height == 600, "Expected resize events to update the SDL window snapshot.");
    success &= Expect(windowState.resizedThisFrame, "Expected resize events to set the resizedThisFrame flag.");

    window.PumpEvents();

    const she::ButtonState heldKeyState = window.GetKeyState(she::KeyCode::W);
    const she::ButtonState heldPointerButtonState = window.GetPointerButtonState(she::PointerButton::Left);
    const she::PointerState clearedPointerState = window.GetPointerState();
    const she::WindowState clearedWindowState = window.GetWindowState();

    success &= Expect(heldKeyState.isDown, "Expected held keys to stay down across frames.");
    success &= Expect(!heldKeyState.pressed && !heldKeyState.released, "Expected key edge flags to clear on the next pump.");
    success &= Expect(heldPointerButtonState.isDown, "Expected held mouse buttons to stay down across frames.");
    success &= Expect(
        !heldPointerButtonState.pressed && !heldPointerButtonState.released,
        "Expected mouse button edge flags to clear on the next pump.");
    success &= Expect(
        clearedPointerState.deltaX == 0 && clearedPointerState.deltaY == 0 && clearedPointerState.wheelX == 0.0F &&
            clearedPointerState.wheelY == 0.0F,
        "Expected per-frame pointer deltas to reset after PumpEvents.");
    success &= Expect(!clearedWindowState.resizedThisFrame, "Expected resizedThisFrame to reset after the next pump.");

    SDL_Event keyUp{};
    keyUp.type = SDL_EVENT_KEY_UP;
    keyUp.key.windowID = initialState.windowId;
    keyUp.key.scancode = SDL_SCANCODE_W;

    SDL_Event mouseUp{};
    mouseUp.type = SDL_EVENT_MOUSE_BUTTON_UP;
    mouseUp.button.windowID = initialState.windowId;
    mouseUp.button.button = SDL_BUTTON_LEFT;

    success &= PushEvent(keyUp);
    success &= PushEvent(mouseUp);

    window.PumpEvents();

    const she::ButtonState releasedKeyState = window.GetKeyState(she::KeyCode::W);
    const she::ButtonState releasedPointerButtonState = window.GetPointerButtonState(she::PointerButton::Left);

    success &= Expect(!releasedKeyState.isDown, "Expected W to stop being held after key up.");
    success &= Expect(releasedKeyState.released, "Expected W to expose a release edge for one frame.");
    success &= Expect(!releasedPointerButtonState.isDown, "Expected the left mouse button to stop being held after button up.");
    success &= Expect(releasedPointerButtonState.released, "Expected the left mouse button to expose a release edge for one frame.");

    window.Shutdown();
    return success;
}
} // namespace

int main(int, char**)
{
    const bool lifecyclePassed = TestWindowLifecycleAndCloseSignal();
    const bool inputPassed = TestInputSnapshotsTrackEdgesAndPointerData();
    return lifecyclePassed && inputPassed ? 0 : 1;
}
