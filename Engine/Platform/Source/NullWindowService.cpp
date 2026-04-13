#include "SHE/Platform/NullWindowService.hpp"

#include "SHE/Core/Logger.hpp"

namespace she
{
void NullWindowService::Initialize(const ApplicationConfig& config)
{
    m_title = config.applicationName;
    m_shouldClose = false;
    m_pumpCount = 0;
    m_windowState.windowId = 0;
    m_windowState.width = config.windowWidth;
    m_windowState.height = config.windowHeight;
    m_windowState.hasKeyboardFocus = false;
    m_windowState.hasMouseFocus = false;
    m_windowState.minimized = config.startWindowHidden;
    m_windowState.closeRequested = false;
    m_windowState.resizedThisFrame = false;

    SHE_LOG_INFO("Platform", "Null window initialized for bootstrap runtime.");
}

void NullWindowService::PumpEvents()
{
    ++m_pumpCount;
    m_windowState.resizedThisFrame = false;
}

bool NullWindowService::ShouldClose() const
{
    return m_shouldClose;
}

void NullWindowService::RequestClose()
{
    m_shouldClose = true;
    m_windowState.closeRequested = true;
}

ButtonState NullWindowService::GetKeyState(const KeyCode) const
{
    return {};
}

ButtonState NullWindowService::GetPointerButtonState(const PointerButton) const
{
    return {};
}

PointerState NullWindowService::GetPointerState() const
{
    return {};
}

WindowState NullWindowService::GetWindowState() const
{
    return m_windowState;
}

std::size_t NullWindowService::GetPumpCount() const
{
    return m_pumpCount;
}

void NullWindowService::Shutdown()
{
    SHE_LOG_INFO("Platform", "Null window shutdown complete.");
}
} // namespace she

