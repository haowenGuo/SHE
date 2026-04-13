#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <array>
#include <string>

struct SDL_Window;

namespace she
{
class SdlWindowService final : public IWindowService
{
public:
    void Initialize(const ApplicationConfig& config) override;
    void PumpEvents() override;
    [[nodiscard]] bool ShouldClose() const override;
    void RequestClose() override;
    [[nodiscard]] ButtonState GetKeyState(KeyCode key) const override;
    [[nodiscard]] ButtonState GetPointerButtonState(PointerButton button) const override;
    [[nodiscard]] PointerState GetPointerState() const override;
    [[nodiscard]] WindowState GetWindowState() const override;
    [[nodiscard]] std::size_t GetPumpCount() const override;
    void Shutdown() override;

private:
    static constexpr std::size_t kKeyCount = static_cast<std::size_t>(KeyCode::Count);
    static constexpr std::size_t kPointerButtonCount = static_cast<std::size_t>(PointerButton::Count);

    void ResetFrameSnapshot();
    void UpdateWindowStateFromSdl();

    SDL_Window* m_window = nullptr;
    std::array<ButtonState, kKeyCount> m_keyStates{};
    std::array<ButtonState, kPointerButtonCount> m_pointerButtonStates{};
    PointerState m_pointerState{};
    WindowState m_windowState{};
    std::string m_title;
    bool m_sdlInitialized = false;
    bool m_shouldClose = false;
    std::size_t m_pumpCount = 0;
};
} // namespace she
