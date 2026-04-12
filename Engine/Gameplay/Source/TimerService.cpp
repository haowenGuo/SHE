#include "SHE/Gameplay/TimerService.hpp"

#include <algorithm>
#include <utility>

namespace she
{
void TimerService::Reset()
{
    m_timers.clear();
}

void TimerService::CreateTimer(std::string timerName, const double durationSeconds, const bool repeating)
{
    m_timers.push_back(GameplayTimer{
        std::move(timerName),
        durationSeconds,
        durationSeconds,
        repeating,
    });
}

std::vector<TimerSignal> TimerService::Advance(const double deltaSeconds)
{
    std::vector<TimerSignal> signals;

    for (auto& timer : m_timers)
    {
        timer.remainingSeconds -= deltaSeconds;
        if (timer.remainingSeconds <= 0.0)
        {
            signals.push_back(TimerSignal{timer.timerName, timer.durationSeconds});
            if (timer.repeating)
            {
                timer.remainingSeconds += timer.durationSeconds;
            }
        }
    }

    m_timers.erase(
        std::remove_if(
            m_timers.begin(),
            m_timers.end(),
            [](const GameplayTimer& timer)
            {
                return !timer.repeating && timer.remainingSeconds <= 0.0;
            }),
        m_timers.end());

    return signals;
}

std::size_t TimerService::GetTimerCount() const
{
    return m_timers.size();
}
} // namespace she
