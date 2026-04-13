#include <SDL3/SDL_main.h>

#include "SHE/Gameplay/GameplayService.hpp"

#include <iostream>
#include <string>

namespace
{
bool Expect(const bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
    }

    return condition;
}

bool Contains(const std::string& value, const std::string& needle)
{
    return value.find(needle) != std::string::npos;
}

bool TestRegisteredCommandRoutesToStructuredEvent()
{
    she::GameplayService gameplay;
    gameplay.Initialize();
    gameplay.RegisterCommand(
        {"SpawnEncounter", "Routes encounter spawns through the gameplay command path.", "encounter", "SpawnRequested"});

    std::size_t receivedEvents = 0;
    std::string lastPayload;
    std::string lastSource;

    const std::size_t routedSubscriptionId = gameplay.SubscribeToEvent(
        "encounter",
        "SpawnRequested",
        [&receivedEvents, &lastPayload, &lastSource](const she::GameplayEvent& event)
        {
            ++receivedEvents;
            lastPayload = event.payload;
            lastSource = event.source;
        });

    gameplay.BeginFrame(4);
    gameplay.AdvanceFrame(1.0 / 60.0);
    gameplay.QueueCommand("SpawnEncounter", "encounter=bootstrap_room_a; enemy=training_dummy");
    gameplay.FlushCommands();

    const std::string digest = gameplay.BuildGameplayDigest();

    bool success = true;
    success = Expect(gameplay.HasCommand("SpawnEncounter"), "Expected SpawnEncounter to be present in the command registry.") && success;
    success = Expect(receivedEvents == 1, "Expected exactly one routed encounter event from the command flush.") && success;
    success = Expect(lastPayload == "encounter=bootstrap_room_a; enemy=training_dummy", "Expected routed command payload to stay intact.") &&
              success;
    success =
        Expect(lastSource == "command:SpawnEncounter", "Expected routed event to retain the originating command source.") && success;
    success = Expect(Contains(digest, "SpawnEncounter => [encounter] SpawnRequested"), "Expected digest to describe registered commands.") &&
              success;
    success = Expect(Contains(digest, "[encounter] SpawnRequested"), "Expected digest to include the published routed event.") && success;

    gameplay.UnsubscribeFromEvent(routedSubscriptionId);
    gameplay.Shutdown();
    return success;
}

bool TestTimerDispatchUsesTheEventBus()
{
    she::GameplayService gameplay;
    gameplay.Initialize();

    std::size_t timerEvents = 0;
    std::size_t followUpEvents = 0;

    const std::size_t timerSubscriptionId = gameplay.SubscribeToEvent(
        "timer",
        "TimerElapsed",
        [&gameplay, &timerEvents](const she::GameplayEvent& event)
        {
            ++timerEvents;
            gameplay.QueueEvent("feature", "TimerObserved", event.payload);
        });

    const std::size_t followUpSubscriptionId = gameplay.SubscribeToEvent(
        "feature",
        "TimerObserved",
        [&followUpEvents](const she::GameplayEvent&)
        {
            ++followUpEvents;
        });

    gameplay.BeginFrame(7);
    gameplay.CreateTimer("contract.timer", 0.03, false);
    gameplay.AdvanceFixedStep(0.015);
    gameplay.AdvanceFixedStep(0.015);
    gameplay.FlushCommands();

    bool success = true;
    success = Expect(timerEvents == 1, "Expected timer dispatch to publish exactly one TimerElapsed event.") && success;
    success = Expect(followUpEvents == 1, "Expected follow-up events queued by a timer subscriber to dispatch in the same flush.") &&
              success;
    success = Expect(gameplay.GetTimerCount() == 0, "Expected one-shot timers to be removed after they elapse.") && success;

    gameplay.UnsubscribeFromEvent(followUpSubscriptionId);
    gameplay.UnsubscribeFromEvent(timerSubscriptionId);
    gameplay.Shutdown();
    return success;
}

bool TestCommandsQueuedDuringDispatchWaitForTheNextFlush()
{
    she::GameplayService gameplay;
    gameplay.Initialize();
    gameplay.RegisterCommand({"SpawnEncounter", "Initial encounter request.", "encounter", "SpawnRequested"});
    gameplay.RegisterCommand({"SpawnFollowUp", "Follow-up encounter request.", "encounter", "FollowUpRequested"});

    std::size_t firstStageEvents = 0;
    std::size_t secondStageEvents = 0;

    const std::size_t firstStageSubscriptionId = gameplay.SubscribeToEvent(
        "encounter",
        "SpawnRequested",
        [&gameplay, &firstStageEvents](const she::GameplayEvent&)
        {
            ++firstStageEvents;
            gameplay.QueueCommand("SpawnFollowUp", "encounter=bootstrap_room_b; trigger=follow_up");
        });

    const std::size_t secondStageSubscriptionId = gameplay.SubscribeToEvent(
        "encounter",
        "FollowUpRequested",
        [&secondStageEvents](const she::GameplayEvent&)
        {
            ++secondStageEvents;
        });

    gameplay.BeginFrame(10);
    gameplay.QueueCommand("SpawnEncounter", "encounter=bootstrap_room_a");
    gameplay.FlushCommands();

    bool success = true;
    success = Expect(firstStageEvents == 1, "Expected the first command to execute during the first flush.") && success;
    success = Expect(secondStageEvents == 0, "Expected commands queued during dispatch to wait until the next flush.") && success;
    success = Expect(gameplay.GetPendingCommandCount() == 1, "Expected the follow-up command to remain pending after the first flush.") &&
              success;

    gameplay.BeginFrame(11);
    gameplay.FlushCommands();

    success = Expect(secondStageEvents == 1, "Expected the follow-up command to execute on the next flush.") && success;
    success = Expect(gameplay.GetPendingCommandCount() == 0, "Expected no pending commands after the second flush.") && success;

    gameplay.UnsubscribeFromEvent(secondStageSubscriptionId);
    gameplay.UnsubscribeFromEvent(firstStageSubscriptionId);
    gameplay.Shutdown();
    return success;
}
} // namespace

int main(int, char**)
{
    bool success = true;
    success = TestRegisteredCommandRoutesToStructuredEvent() && success;
    success = TestTimerDispatchUsesTheEventBus() && success;
    success = TestCommandsQueuedDuringDispatchWaitForTheNextFlush() && success;
    return success ? 0 : 1;
}
