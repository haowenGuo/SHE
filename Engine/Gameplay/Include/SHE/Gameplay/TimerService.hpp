#pragma once

#include <string>
#include <vector>

namespace she
{
struct TimerSignal
{
    std::string timerName;
    double durationSeconds = 0.0;
};

struct GameplayTimer
{
    std::string timerName;
    double remainingSeconds = 0.0;
    double durationSeconds = 0.0;
    bool repeating = false;
};

class TimerService
{
public:
    void Reset();
    void CreateTimer(std::string timerName, double durationSeconds, bool repeating);
    [[nodiscard]] std::vector<TimerSignal> Advance(double deltaSeconds);
    [[nodiscard]] std::size_t GetTimerCount() const;

private:
    std::vector<GameplayTimer> m_timers;
};
} // namespace she

