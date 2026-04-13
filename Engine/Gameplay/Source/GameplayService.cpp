#include "SHE/Gameplay/GameplayService.hpp"

#include "SHE/Core/Logger.hpp"

#include <sstream>
#include <string_view>
#include <utility>

namespace she
{
void GameplayService::Initialize()
{
    m_frameIndex = 0;
    m_lastDeltaSeconds = 0.0;
    m_commandRegistry.Reset();
    m_eventBus.Reset();
    m_commandBuffer.Reset();
    m_timerService.Reset();
    SHE_LOG_INFO("Gameplay", "Gameplay service initialized.");
}

void GameplayService::Shutdown()
{
    SHE_LOG_INFO("Gameplay", "Gameplay service shutdown complete.");
    m_commandRegistry.Reset();
    m_eventBus.Reset();
    m_commandBuffer.Reset();
    m_timerService.Reset();
}

void GameplayService::RegisterCommand(GameplayCommandDescriptor descriptor)
{
    if (descriptor.name.empty())
    {
        SHE_LOG_WARNING("Gameplay", "Ignored gameplay command registration because the command name was empty.");
        return;
    }

    if (descriptor.routedEventCategory.empty())
    {
        descriptor.routedEventCategory = "command";
    }

    if (descriptor.routedEventName.empty())
    {
        descriptor.routedEventName = "Executed";
    }

    m_commandRegistry.Register(std::move(descriptor));
}

bool GameplayService::HasCommand(const std::string_view commandName) const
{
    return m_commandRegistry.Find(commandName) != nullptr;
}

std::size_t GameplayService::SubscribeToEvent(
    std::string category,
    std::string name,
    GameplayEventHandler handler)
{
    return m_eventBus.Subscribe(std::move(category), std::move(name), std::move(handler));
}

void GameplayService::UnsubscribeFromEvent(const std::size_t subscriptionId)
{
    m_eventBus.Unsubscribe(subscriptionId);
}

void GameplayService::BeginFrame(const FrameIndex frameIndex)
{
    m_frameIndex = frameIndex;
    m_eventBus.BeginFrame(frameIndex);
}

void GameplayService::AdvanceFixedStep(const double fixedTimeStep)
{
    const auto signals = m_timerService.Advance(fixedTimeStep);
    for (const auto& signal : signals)
    {
        PublishEvent("timer", "TimerElapsed", BuildTimerPayload(signal), BuildTimerSource(signal.timerName));
    }
}

void GameplayService::AdvanceFrame(const double deltaSeconds)
{
    m_lastDeltaSeconds = deltaSeconds;
}

void GameplayService::QueueEvent(std::string category, std::string name, std::string payload)
{
    PublishEvent(std::move(category), std::move(name), std::move(payload), "gameplay_api");
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
        ExecuteCommand(command);
    }

    m_eventBus.DispatchAll();
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
    stream << "frame=" << m_frameIndex << ", delta=" << m_lastDeltaSeconds
           << ", registered_commands=" << m_commandRegistry.GetCount() << ", timers=" << GetTimerCount()
           << ", pending_commands=" << GetPendingCommandCount()
           << ", published_events=" << m_eventBus.GetPublishedThisFrame().size() << '\n';

    stream << "[command_catalog]\n" << m_commandRegistry.BuildCatalog();
    stream << "[published_events]\n";

    const auto& publishedEvents = m_eventBus.GetPublishedThisFrame();
    if (publishedEvents.empty())
    {
        stream << "(none)\n";
        return stream.str();
    }

    for (const auto& event : publishedEvents)
    {
        stream << "- [" << event.category << "] " << event.name << " (source=" << event.source << ", frame=" << event.frameIndex
               << "): " << event.payload << '\n';
    }

    return stream.str();
}

void GameplayService::ExecuteCommand(const GameplayCommand& command)
{
    const GameplayCommandDescriptor* descriptor = m_commandRegistry.Find(command.name);
    if (descriptor == nullptr)
    {
        PublishEvent(
            "command",
            "Unregistered",
            "command=" + command.name + "; payload=" + command.payload,
            BuildCommandSource(command.name));
        SHE_LOG_WARNING("Gameplay", "Queued gameplay command had no registered descriptor.");
        return;
    }

    PublishEvent(
        descriptor->routedEventCategory,
        descriptor->routedEventName,
        command.payload,
        BuildCommandSource(command.name));
}

void GameplayService::PublishEvent(std::string category, std::string name, std::string payload, std::string source)
{
    m_eventBus.Enqueue(GameplayEvent{
        std::move(category),
        std::move(name),
        std::move(payload),
        std::move(source),
        m_frameIndex,
    });
}

std::string GameplayService::BuildTimerPayload(const TimerSignal& signal)
{
    return "timer=" + signal.timerName + "; duration_seconds=" + std::to_string(signal.durationSeconds) +
           "; repeating=" + std::string(signal.repeating ? "true" : "false");
}

std::string GameplayService::BuildCommandSource(const std::string_view commandName)
{
    return "command:" + std::string(commandName);
}

std::string GameplayService::BuildTimerSource(const std::string_view timerName)
{
    return "timer:" + std::string(timerName);
}
} // namespace she
