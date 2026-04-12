#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace she
{
struct DataSchema
{
    std::string schemaName;
    std::string description;
    std::vector<std::string> requiredFields;
};

// DataService gives gameplay a schema-first home. The first milestone does not
// parse YAML yet, but it does encode the rule that gameplay data should be
// described, validated, and documented centrally rather than being ad hoc.
class DataService final : public IDataService
{
public:
    void Initialize() override;
    void Shutdown() override;
    void RegisterSchema(std::string schemaName, std::string description, std::vector<std::string> requiredFields) override;
    [[nodiscard]] bool HasSchema(std::string_view schemaName) const override;
    [[nodiscard]] std::size_t GetSchemaCount() const override;
    [[nodiscard]] std::string DescribeSchemas() const override;

private:
    std::unordered_map<std::string, DataSchema> m_schemas;
};
} // namespace she

