#pragma once

#include "SHE/Core/RuntimeServices.hpp"
#include "SHE/Gameplay/CommandBuffer.hpp"
#include "SHE/Gameplay/TimerService.hpp"

#include <string>
#include <vector>

namespace she
{
struct GameplayEvent
{
    std::string category;
    std::string name;
    std::string payload;
};

// GameplayService is the main AI-native gameplay substrate.
// Instead of asking every feature to invent its own timing, event, and command
// rules, we centralize those primitives here so Codex and human contributors
// can build gameplay by composition.
class GameplayService final : public IGameplayService
{
public:
    void Initialize() override;
    void Shutdown() override;
    void BeginFrame(FrameIndex frameIndex) override;
    void AdvanceFixedStep(double fixedTimeStep) override;
    void AdvanceFrame(double deltaSeconds) override;
    void QueueEvent(std::string category, std::string name, std::string payload) override;
    void QueueCommand(std::string name, std::string payload) override;
    void CreateTimer(std::string timerName, double durationSeconds, bool repeating) override;
    void FlushCommands() override;
    [[nodiscard]] std::size_t GetPendingCommandCount() const override;
    [[nodiscard]] std::size_t GetTimerCount() const override;
    [[nodiscard]] std::string BuildGameplayDigest() const override;

private:
    FrameIndex m_frameIndex = 0;
    double m_lastDeltaSeconds = 0.0;
    std::vector<GameplayEvent> m_publishedEvents;
    CommandBuffer m_commandBuffer;
    TimerService m_timerService;
};
} // namespace she
