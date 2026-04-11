#pragma once

#include <chrono>
#include <cstddef>

namespace she
{
struct FrameSchedule
{
    double deltaSeconds = 0.0;
    std::size_t fixedSteps = 0;
};

// AppClock is responsible for translating wall-clock time into a frame schedule.
// In Phase 1 it can run in deterministic mode so tests and bootstrap executables
// produce stable results before a real platform timer is integrated.
class AppClock
{
public:
    AppClock(double fixedTimeStep, double maxFrameDelta, double targetFrameRate, bool deterministicTiming);

    void Reset();
    [[nodiscard]] FrameSchedule AdvanceFrame();
    [[nodiscard]] double GetFixedTimeStep() const;

private:
    double m_fixedTimeStep = 1.0 / 60.0;
    double m_maxFrameDelta = 0.25;
    double m_targetFrameDuration = 1.0 / 60.0;
    bool m_deterministicTiming = true;
    double m_accumulator = 0.0;
    std::chrono::steady_clock::time_point m_lastTick = {};
};
} // namespace she

