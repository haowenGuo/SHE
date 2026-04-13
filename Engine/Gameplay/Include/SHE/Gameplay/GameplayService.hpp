#pragma once

#include "SHE/Core/RuntimeServices.hpp"
#include "SHE/Gameplay/CommandBuffer.hpp"
#include "SHE/Gameplay/CommandRegistry.hpp"
#include "SHE/Gameplay/EventBus.hpp"
#include "SHE/Gameplay/TimerService.hpp"

namespace she
{
// GameplayService is the main AI-native gameplay substrate.
// Instead of asking every feature to invent its own timing, event, and command
// rules, we centralize those primitives here so Codex and human contributors
// can build gameplay by composition.
class GameplayService final : public IGameplayService
{
public:
    void Initialize() override;
    void Shutdown() override;
    void RegisterCommand(GameplayCommandDescriptor descriptor) override;
    [[nodiscard]] bool HasCommand(std::string_view commandName) const override;
    [[nodiscard]] std::size_t SubscribeToEvent(
        std::string category,
        std::string name,
        GameplayEventHandler handler) override;
    void UnsubscribeFromEvent(std::size_t subscriptionId) override;
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
    // FlushCommands snapshots the current command buffer, routes those commands
    // into events, and then drains the event queue FIFO. Event handlers may
    // enqueue more events for the same flush, but commands queued by handlers
    // intentionally wait for the next flush to keep execution order stable.
    void ExecuteCommand(const GameplayCommand& command);
    void PublishEvent(std::string category, std::string name, std::string payload, std::string source);
    [[nodiscard]] static std::string BuildTimerPayload(const TimerSignal& signal);
    [[nodiscard]] static std::string BuildCommandSource(std::string_view commandName);
    [[nodiscard]] static std::string BuildTimerSource(std::string_view timerName);

    FrameIndex m_frameIndex = 0;
    double m_lastDeltaSeconds = 0.0;
    CommandRegistry m_commandRegistry;
    EventBus m_eventBus;
    CommandBuffer m_commandBuffer;
    TimerService m_timerService;
};
} // namespace she
