#include "SHE/Gameplay/CommandBuffer.hpp"

#include <utility>

namespace she
{
void CommandBuffer::Reset()
{
    m_pending.clear();
}

void CommandBuffer::Queue(GameplayCommand command)
{
    m_pending.push_back(std::move(command));
}

std::vector<GameplayCommand> CommandBuffer::ConsumeAll()
{
    std::vector<GameplayCommand> commands = std::move(m_pending);
    m_pending.clear();
    return commands;
}

std::size_t CommandBuffer::GetPendingCount() const
{
    return m_pending.size();
}
} // namespace she

