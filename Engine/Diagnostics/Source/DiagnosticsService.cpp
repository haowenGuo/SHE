#include "SHE/Diagnostics/DiagnosticsService.hpp"

#include "SHE/Core/Logger.hpp"

#include <sstream>
#include <utility>

namespace she
{
void DiagnosticsService::Initialize()
{
    m_currentTrace = {};
    m_history.clear();
    SHE_LOG_INFO("Diagnostics", "Diagnostics service initialized.");
}

void DiagnosticsService::Shutdown()
{
    SHE_LOG_INFO("Diagnostics", "Diagnostics service shutdown complete.");
    m_currentTrace = {};
    m_history.clear();
}

void DiagnosticsService::BeginFrame(const FrameIndex frameIndex)
{
    m_currentTrace = FrameTrace{frameIndex, {}};
}

void DiagnosticsService::RecordPhase(std::string phaseName, std::string summary)
{
    m_currentTrace.phases.push_back(FramePhaseTrace{std::move(phaseName), std::move(summary)});
}

void DiagnosticsService::EndFrame()
{
    if (m_currentTrace.phases.empty())
    {
        return;
    }

    m_history.push_back(m_currentTrace);
    if (m_history.size() > 32)
    {
        m_history.erase(m_history.begin());
    }
}

std::size_t DiagnosticsService::GetCapturedFrameCount() const
{
    return m_history.size();
}

std::string DiagnosticsService::BuildLatestReport() const
{
    if (m_history.empty())
    {
        return "No diagnostic frames captured yet.\n";
    }

    const FrameTrace& trace = m_history.back();
    std::ostringstream stream;
    stream << "frame=" << trace.frameIndex << '\n';
    for (const auto& phase : trace.phases)
    {
        stream << "- " << phase.phaseName << ": " << phase.summary << '\n';
    }

    return stream.str();
}
} // namespace she

