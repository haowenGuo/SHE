#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <string>
#include <vector>

namespace she
{
struct FramePhaseTrace
{
    std::string phaseName;
    std::string summary;
};

struct FrameTrace
{
    FrameIndex frameIndex = 0;
    std::vector<FramePhaseTrace> phases;
};

// DiagnosticsService records a compact frame narrative. This is intentionally
// simple but already useful for AI-assisted debugging because it explains what
// the engine believed happened in a frame.
class DiagnosticsService final : public IDiagnosticsService
{
public:
    void Initialize() override;
    void Shutdown() override;
    void BeginFrame(FrameIndex frameIndex) override;
    void RecordPhase(std::string phaseName, std::string summary) override;
    void EndFrame() override;
    [[nodiscard]] std::size_t GetCapturedFrameCount() const override;
    [[nodiscard]] std::string BuildLatestReport() const override;

private:
    FrameTrace m_currentTrace;
    std::vector<FrameTrace> m_history;
};
} // namespace she

