#include "SHE/Gameplay/EventBus.hpp"

#include <algorithm>
#include <utility>

namespace she
{
void EventBus::Reset()
{
    m_frameIndex = 0;
    m_nextSubscriptionId = 1;
    m_pendingEvents.clear();
    m_publishedThisFrame.clear();
    m_subscriptions.clear();
}

void EventBus::BeginFrame(const FrameIndex frameIndex)
{
    m_frameIndex = frameIndex;
    m_publishedThisFrame.clear();
}

void EventBus::Enqueue(GameplayEvent event)
{
    event.frameIndex = m_frameIndex;
    m_pendingEvents.push_back(std::move(event));
}

std::size_t EventBus::Subscribe(std::string category, std::string name, GameplayEventHandler handler)
{
    const std::size_t subscriptionId = m_nextSubscriptionId++;
    m_subscriptions.push_back(EventSubscription{
        subscriptionId,
        std::move(category),
        std::move(name),
        std::move(handler),
    });
    return subscriptionId;
}

void EventBus::Unsubscribe(const std::size_t subscriptionId)
{
    m_subscriptions.erase(
        std::remove_if(
            m_subscriptions.begin(),
            m_subscriptions.end(),
            [subscriptionId](const EventSubscription& subscription)
            {
                return subscription.id == subscriptionId;
            }),
        m_subscriptions.end());
}

void EventBus::DispatchAll()
{
    while (!m_pendingEvents.empty())
    {
        GameplayEvent event = std::move(m_pendingEvents.front());
        m_pendingEvents.pop_front();
        m_publishedThisFrame.push_back(event);

        std::vector<GameplayEventHandler> matchingHandlers;
        matchingHandlers.reserve(m_subscriptions.size());

        for (const auto& subscription : m_subscriptions)
        {
            if (Matches(subscription.category, event.category) && Matches(subscription.name, event.name))
            {
                matchingHandlers.push_back(subscription.handler);
            }
        }

        for (const auto& handler : matchingHandlers)
        {
            handler(event);
        }
    }
}

const std::vector<GameplayEvent>& EventBus::GetPublishedThisFrame() const
{
    return m_publishedThisFrame;
}

bool EventBus::Matches(const std::string_view pattern, const std::string_view value)
{
    return pattern.empty() || pattern == "*" || pattern == value;
}
} // namespace she
