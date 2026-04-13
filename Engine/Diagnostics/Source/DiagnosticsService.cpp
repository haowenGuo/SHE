#include "SHE/Diagnostics/DiagnosticsService.hpp"

#include "SHE/Core/Logger.hpp"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace she
{
namespace
{
std::size_t CountNonEmptyLines(const std::string_view text)
{
    if (text.empty())
    {
        return 0;
    }

    std::size_t count = 0;
    std::istringstream stream(std::string{text});
    std::string line;
    while (std::getline(stream, line))
    {
        if (!line.empty())
        {
            ++count;
        }
    }

    return count;
}

std::string JoinPhaseNames(const std::vector<FramePhaseTrace>& phases)
{
    std::ostringstream stream;
    for (std::size_t index = 0; index < phases.size(); ++index)
    {
        stream << phases[index].phaseName;
        if (index + 1 < phases.size())
        {
            stream << ", ";
        }
    }

    return stream.str();
}

bool ContainsGameplayActivity(const std::vector<FramePhaseTrace>& phases)
{
    return std::any_of(
        phases.begin(),
        phases.end(),
        [](const FramePhaseTrace& phase)
        {
            return phase.phaseName == "Gameplay" &&
                   (phase.summary.find("- [") != std::string::npos || phase.summary.find("pending_commands=") != std::string::npos);
        });
}

void AppendIndentedBlock(std::ostringstream& stream, const std::string_view text)
{
    if (text.empty())
    {
        stream << "  <empty>\n";
        return;
    }

    std::istringstream block(std::string{text});
    std::string line;
    while (std::getline(block, line))
    {
        stream << "  " << line << '\n';
    }
}
} // namespace

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

    m_history.push_back(std::move(m_currentTrace));
    if (m_history.size() > kMaxHistory)
    {
        m_history.erase(m_history.begin());
    }

    m_currentTrace = {};
}

std::size_t DiagnosticsService::GetCapturedFrameCount() const
{
    return m_history.size();
}

std::string DiagnosticsService::BuildLatestReport() const
{
    if (m_history.empty())
    {
        return "diagnostics_report_version: 1\ncaptured_frames: 0\nstatus: empty\n";
    }

    const FrameTrace& trace = m_history.back();
    std::ostringstream stream;
    stream << "diagnostics_report_version: 1\n";
    stream << "captured_frames: " << m_history.size() << '\n';
    stream << "frame_index: " << trace.frameIndex << '\n';
    stream << "phase_count: " << trace.phases.size() << '\n';
    stream << "contains_gameplay_activity: " << (ContainsGameplayActivity(trace.phases) ? "true" : "false") << '\n';
    stream << "\n[frame_summary]\n";
    stream << "phase_names: " << JoinPhaseNames(trace.phases) << '\n';

    for (std::size_t index = 0; index < trace.phases.size(); ++index)
    {
        const auto& phase = trace.phases[index];
        stream << "\n[phase_" << index << "]\n";
        stream << "name: " << phase.phaseName << '\n';
        stream << "detail_line_count: " << CountNonEmptyLines(phase.summary) << '\n';
        stream << "detail:\n";
        AppendIndentedBlock(stream, phase.summary);
    }

    return stream.str();
}
} // namespace she

