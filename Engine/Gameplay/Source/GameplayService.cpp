#include "SHE/Gameplay/GameplayService.hpp"

#include "SHE/Core/Logger.hpp"

#include <sstream>
#include <utility>

namespace she
{
void GameplayService::Initialize()
{
    m_frameIndex = 0;
    m_lastDeltaSeconds = 0.0;
    m_publishedEvents.clear();
    m_timerService.Reset();
    SHE_LOG_INFO("Gameplay", "Gameplay service initialized.");
}

void GameplayService::Shutdown()
{
    SHE_LOG_INFO("Gameplay", "Gameplay service shutdown complete.");
    m_publishedEvents.clear();
    m_timerService.Reset();
}

void GameplayService::BeginFrame(const FrameIndex frameIndex)
{
    m_frameIndex = frameIndex;
    m_publishedEvents.clear();
}

void GameplayService::AdvanceFixedStep(const double fixedTimeStep)
{
    const auto signals = m_timerService.Advance(fixedTimeStep);
    for (const auto& signal : signals)
    {
        m_publishedEvents.push_back(
            GameplayEvent{"timer", "TimerElapsed", signal.timerName + " elapsed after " + std::to_string(signal.durationSeconds)});
    }
}

void GameplayService::AdvanceFrame(const double deltaSeconds)
{
    m_lastDeltaSeconds = deltaSeconds;
}

void GameplayService::QueueEvent(std::string category, std::string name, std::string payload)
{
    m_publishedEvents.push_back(GameplayEvent{std::move(category), std::move(name), std::move(payload)});
}

void GameplayService::QueueCommand(std::string name, std::string payload)
{
    m_commandBuffer.Queue(GameplayCommand{std::move(name), std::move(payload)});
}

void GameplayService::CreateTimer(std::string timerName, const double durationSeconds, const bool repeating)
{
    m_timerService.CreateTimer(std::move(timerName), durationSeconds, repeating);
}

void GameplayService::FlushCommands()
{
    const auto commands = m_commandBuffer.ConsumeAll();
    for (const auto& command : commands)
    {
        m_publishedEvents.push_back(GameplayEvent{"command", command.name, command.payload});
    }
}

std::size_t GameplayService::GetPendingCommandCount() const
{
    return m_commandBuffer.GetPendingCount();
}

std::size_t GameplayService::GetTimerCount() const
{
    return m_timerService.GetTimerCount();
}

std::string GameplayService::BuildGameplayDigest() const
{
    std::ostringstream stream;
    stream << "frame=" << m_frameIndex << ", delta=" << m_lastDeltaSeconds << ", timers=" << GetTimerCount()
           << ", pending_commands=" << GetPendingCommandCount() << '\n';
    for (const auto& event : m_publishedEvents)
    {
        stream << "- [" << event.category << "] " << event.name << ": " << event.payload << '\n';
    }

    return stream.str();
}
} // namespace she
