#include "SHE/UI/NullUiService.hpp"

#include "SHE/Core/Logger.hpp"

#include <sstream>

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

std::string NullUiService::BuildLatestDebugReport() const
{
    std::ostringstream stream;
    stream << "ui_debug_report_version: 1\n";
    stream << "implementation: null\n";
    stream << "frame_index: " << m_lastFrameIndex << '\n';
    stream << "status: unavailable\n";
    return stream.str();
}
} // namespace she
