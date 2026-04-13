#include "SHE/Gameplay/TimerService.hpp"

#include "SHE/Core/Logger.hpp"

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
    if (timerName.empty())
    {
        SHE_LOG_WARNING("Gameplay", "Ignored timer creation because the timer name was empty.");
        return;
    }

    if (durationSeconds <= 0.0)
    {
        SHE_LOG_WARNING("Gameplay", "Ignored timer creation because the duration was not positive.");
        return;
    }

    const auto existingTimer = std::find_if(
        m_timers.begin(),
        m_timers.end(),
        [&timerName](const GameplayTimer& timer)
        {
            return timer.timerName == timerName;
        });

    if (existingTimer != m_timers.end())
    {
        *existingTimer = GameplayTimer{
            std::move(timerName),
            durationSeconds,
            durationSeconds,
            repeating,
        };
        return;
    }

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
        if (timer.repeating)
        {
            while (timer.remainingSeconds <= 0.0)
            {
                signals.push_back(TimerSignal{timer.timerName, timer.durationSeconds, true});
                timer.remainingSeconds += timer.durationSeconds;
            }

            continue;
        }

        if (timer.remainingSeconds <= 0.0)
        {
            signals.push_back(TimerSignal{timer.timerName, timer.durationSeconds, false});
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
