#include "SHE/Core/AppClock.hpp"

#include <algorithm>

namespace she
{
AppClock::AppClock(
    const double fixedTimeStep,
    const double maxFrameDelta,
    const double targetFrameRate,
    const bool deterministicTiming)
    : m_fixedTimeStep(fixedTimeStep)
    , m_maxFrameDelta(maxFrameDelta)
    , m_targetFrameDuration(targetFrameRate > 0.0 ? (1.0 / targetFrameRate) : fixedTimeStep)
    , m_deterministicTiming(deterministicTiming)
{
}

void AppClock::Reset()
{
    m_accumulator = 0.0;
    m_lastTick = std::chrono::steady_clock::now();
}

FrameSchedule AppClock::AdvanceFrame()
{
    double deltaSeconds = m_targetFrameDuration;

    if (!m_deterministicTiming)
    {
        const auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<double> delta = now - m_lastTick;
        deltaSeconds = delta.count();
        m_lastTick = now;
    }

    deltaSeconds = std::clamp(deltaSeconds, 0.0, m_maxFrameDelta);
    m_accumulator += deltaSeconds;

    std::size_t fixedSteps = 0;
    while (m_accumulator >= m_fixedTimeStep)
    {
        m_accumulator -= m_fixedTimeStep;
        ++fixedSteps;
    }

    return FrameSchedule{deltaSeconds, fixedSteps};
}

double AppClock::GetFixedTimeStep() const
{
    return m_fixedTimeStep;
}
} // namespace she

