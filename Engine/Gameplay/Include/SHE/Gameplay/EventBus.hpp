#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <deque>
#include <string>
#include <string_view>
#include <vector>

namespace she
{
// EventBus owns one frame-scoped publication history plus a pending queue.
// Events queued while dispatching are drained in the same flush, while commands
// queued by handlers wait for the next FlushCommands call in GameplayService.
class EventBus
{
public:
    void Reset();
    void BeginFrame(FrameIndex frameIndex);
    void Enqueue(GameplayEvent event);
    [[nodiscard]] std::size_t Subscribe(std::string category, std::string name, GameplayEventHandler handler);
    void Unsubscribe(std::size_t subscriptionId);
    void DispatchAll();
    [[nodiscard]] const std::vector<GameplayEvent>& GetPublishedThisFrame() const;

private:
    struct EventSubscription
    {
        std::size_t id = 0;
        std::string category;
        std::string name;
        GameplayEventHandler handler;
    };

    [[nodiscard]] static bool Matches(std::string_view pattern, std::string_view value);

    FrameIndex m_frameIndex = 0;
    std::size_t m_nextSubscriptionId = 1;
    std::deque<GameplayEvent> m_pendingEvents;
    std::vector<GameplayEvent> m_publishedThisFrame;
    std::vector<EventSubscription> m_subscriptions;
};
} // namespace she
