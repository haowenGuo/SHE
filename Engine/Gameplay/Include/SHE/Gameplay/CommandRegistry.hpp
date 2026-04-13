#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace she
{
// CommandRegistry is intentionally small: gameplay features need a stable place
// to publish command names and routed event metadata before scripting or AI
// tooling starts generating those commands on their behalf.
class CommandRegistry
{
public:
    void Reset();
    void Register(GameplayCommandDescriptor descriptor);
    [[nodiscard]] const GameplayCommandDescriptor* Find(std::string_view name) const;
    [[nodiscard]] std::size_t GetCount() const;
    [[nodiscard]] std::string BuildCatalog() const;

private:
    std::unordered_map<std::string, GameplayCommandDescriptor> m_descriptors;
};
} // namespace she
