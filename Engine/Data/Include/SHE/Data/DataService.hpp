#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <map>

namespace she
{
// DataService gives gameplay a schema-first home. The first milestone does not
// stop at "schema names exist". It owns the shared contract for schema
// registration, constrained YAML loading, structured validation results, and
// the registry queries that other modules can trust.
class DataService final : public IDataService
{
public:
    void Initialize() override;
    void Shutdown() override;
    [[nodiscard]] DataSchemaRegistrationResult RegisterSchema(DataSchemaContract schema) override;
    [[nodiscard]] DataLoadResult LoadRecord(const DataLoadRequest& request) override;
    [[nodiscard]] bool HasSchema(std::string_view schemaName) const override;
    [[nodiscard]] bool HasTrustedRecord(std::string_view recordId) const override;
    [[nodiscard]] std::size_t GetSchemaCount() const override;
    [[nodiscard]] std::size_t GetRecordCount() const override;
    [[nodiscard]] std::size_t GetTrustedRecordCount() const override;
    [[nodiscard]] std::vector<DataSchemaContract> ListSchemas() const override;
    [[nodiscard]] std::vector<DataRegistryEntry> ListRecords() const override;
    [[nodiscard]] std::string DescribeSchemas() const override;
    [[nodiscard]] std::string DescribeRegistry() const override;

private:
    std::map<std::string, DataSchemaContract> m_schemas;
    std::map<std::string, DataRegistryEntry> m_records;
};
} // namespace she

