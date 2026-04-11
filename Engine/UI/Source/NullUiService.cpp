#include "SHE/UI/NullUiService.hpp"

#include "SHE/Core/Logger.hpp"

namespace she
{
void NullUiService::Initialize()
{
    m_lastFrameIndex = 0;
    SHE_LOG_INFO("UI", "Null UI service initialized.");
}

void NullUiService::Shutdown()
{
    SHE_LOG_INFO("UI", "Null UI service shutdown complete.");
}

void NullUiService::BeginFrame(const FrameIndex frameIndex)
{
    m_lastFrameIndex = frameIndex;
}

void NullUiService::EndFrame()
{
}
} // namespace she
