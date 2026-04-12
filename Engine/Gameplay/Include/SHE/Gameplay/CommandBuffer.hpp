#pragma once

#include <string>
#include <vector>

namespace she
{
struct GameplayCommand
{
    std::string name;
    std::string payload;
};

class CommandBuffer
{
public:
    void Queue(GameplayCommand command);
    [[nodiscard]] std::vector<GameplayCommand> ConsumeAll();
    [[nodiscard]] std::size_t GetPendingCount() const;

private:
    std::vector<GameplayCommand> m_pending;
};
} // namespace she

