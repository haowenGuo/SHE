#include "SHE/Reflection/ReflectionService.hpp"

#include "SHE/Core/Logger.hpp"

#include <sstream>
#include <utility>

namespace she
{
void ReflectionService::Initialize()
{
    m_types.clear();
    m_features.clear();
    SHE_LOG_INFO("Reflection", "Reflection service initialized.");
}

void ReflectionService::Shutdown()
{
    SHE_LOG_INFO("Reflection", "Reflection service shutdown complete.");
    m_types.clear();
    m_features.clear();
}

void ReflectionService::RegisterType(std::string category, std::string typeName, std::string description)
{
    m_types.push_back(TypeDescriptor{std::move(category), std::move(typeName), std::move(description)});
}

void ReflectionService::RegisterFeature(std::string featureName, std::string ownerModule, std::string description)
{
    m_features.push_back(FeatureDescriptor{std::move(featureName), std::move(ownerModule), std::move(description)});
}

std::size_t ReflectionService::GetRegisteredTypeCount() const
{
    return m_types.size();
}

std::size_t ReflectionService::GetRegisteredFeatureCount() const
{
    return m_features.size();
}

std::string ReflectionService::BuildTypeCatalog() const
{
    std::ostringstream stream;
    stream << "Types:\n";
    for (const auto& descriptor : m_types)
    {
        stream << "- [" << descriptor.category << "] " << descriptor.typeName << ": " << descriptor.description << '\n';
    }

    stream << "Features:\n";
    for (const auto& descriptor : m_features)
    {
        stream << "- " << descriptor.featureName << " (" << descriptor.ownerModule << "): " << descriptor.description
               << '\n';
    }

    return stream.str();
}
} // namespace she

