#include "SHE/Data/DataService.hpp"

#include "SHE/Core/Logger.hpp"

#include <sstream>
#include <utility>

namespace she
{
void DataService::Initialize()
{
    m_schemas.clear();
    SHE_LOG_INFO("Data", "Data service initialized.");
}

void DataService::Shutdown()
{
    SHE_LOG_INFO("Data", "Data service shutdown complete.");
    m_schemas.clear();
}

void DataService::RegisterSchema(
    std::string schemaName,
    std::string description,
    std::vector<std::string> requiredFields)
{
    m_schemas.insert_or_assign(
        schemaName,
        DataSchema{std::move(schemaName), std::move(description), std::move(requiredFields)});
}

bool DataService::HasSchema(const std::string_view schemaName) const
{
    return m_schemas.contains(std::string(schemaName));
}

std::size_t DataService::GetSchemaCount() const
{
    return m_schemas.size();
}

std::string DataService::DescribeSchemas() const
{
    std::ostringstream stream;
    for (const auto& [name, schema] : m_schemas)
    {
        stream << "- " << name << ": " << schema.description << " | required=";
        for (std::size_t index = 0; index < schema.requiredFields.size(); ++index)
        {
            stream << schema.requiredFields[index];
            if (index + 1 < schema.requiredFields.size())
            {
                stream << ", ";
            }
        }
        stream << '\n';
    }

    return stream.str();
}
} // namespace she

