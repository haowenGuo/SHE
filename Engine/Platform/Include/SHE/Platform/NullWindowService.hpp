#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <string>

namespace she
{
// NullWindowService is the first bootstrap implementation of the platform layer.
// It does not create a real window. Instead it gives the application loop a
// stable lifecycle surface until SDL3 is integrated in the next phase.
class NullWindowService final : public IWindowService
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
    [[nodiscard]] NativeWindowHandle GetNativeWindowHandle() const override;
    [[nodiscard]] std::size_t GetPumpCount() const override;
    void Shutdown() override;

private:
    std::string m_title;
    bool m_shouldClose = false;
    std::size_t m_pumpCount = 0;
    WindowState m_windowState{};
};
} // namespace she

