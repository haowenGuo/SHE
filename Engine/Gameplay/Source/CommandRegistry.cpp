#include "SHE/Gameplay/CommandRegistry.hpp"

#include <sstream>
#include <utility>

namespace she
{
void CommandRegistry::Reset()
{
    m_descriptors.clear();
}

void CommandRegistry::Register(GameplayCommandDescriptor descriptor)
{
    m_descriptors[descriptor.name] = std::move(descriptor);
}

const GameplayCommandDescriptor* CommandRegistry::Find(const std::string_view name) const
{
    const auto descriptor = m_descriptors.find(std::string(name));
    if (descriptor == m_descriptors.end())
    {
        return nullptr;
    }

    return &descriptor->second;
}

std::size_t CommandRegistry::GetCount() const
{
    return m_descriptors.size();
}

std::string CommandRegistry::BuildCatalog() const
{
    if (m_descriptors.empty())
    {
        return "(none)\n";
    }

    std::ostringstream stream;
    for (const auto& [name, descriptor] : m_descriptors)
    {
        stream << "- " << name << " => [" << descriptor.routedEventCategory << "] " << descriptor.routedEventName;
        if (!descriptor.description.empty())
        {
            stream << " :: " << descriptor.description;
        }

        stream << '\n';
    }

    return stream.str();
}
} // namespace she
