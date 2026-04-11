#include "SHE/Platform/NullWindowService.hpp"

#include "SHE/Core/Logger.hpp"

namespace she
{
void NullWindowService::Initialize(const ApplicationConfig& config)
{
    m_title = config.applicationName;
    m_shouldClose = false;
    m_pumpCount = 0;

    SHE_LOG_INFO("Platform", "Null window initialized for bootstrap runtime.");
}

void NullWindowService::PumpEvents()
{
    ++m_pumpCount;
}

bool NullWindowService::ShouldClose() const
{
    return m_shouldClose;
}

void NullWindowService::RequestClose()
{
    m_shouldClose = true;
}

void NullWindowService::Shutdown()
{
    SHE_LOG_INFO("Platform", "Null window shutdown complete.");
}
} // namespace she

