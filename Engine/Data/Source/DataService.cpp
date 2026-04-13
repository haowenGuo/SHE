#include "SHE/Data/DataService.hpp"

#include "SHE/Core/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>

namespace she
{
namespace
{
struct ParsedYamlField
{
    std::string fieldName;
    std::string valueKind;
};

[[nodiscard]] std::string TrimCopy(std::string_view value)
{
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0)
    {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
    {
        --end;
    }

    return std::string(value.substr(begin, end - begin));
}

[[nodiscard]] std::size_t CountIndent(std::string_view value)
{
    std::size_t indent = 0;
    while (indent < value.size() && (value[indent] == ' ' || value[indent] == '\t'))
    {
        ++indent;
    }

    return indent;
}

[[nodiscard]] std::string NormalizeValueKind(std::string_view value)
{
    std::string normalized = TrimCopy(value);
    std::transform(
        normalized.begin(),
        normalized.end(),
        normalized.begin(),
        [](const unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });

    if (normalized.empty())
    {
        return "scalar";
    }

    return normalized;
}

[[nodiscard]] std::string InferInlineValueKind(std::string_view remainder)
{
    const std::string trimmed = TrimCopy(remainder);
    if (trimmed.empty())
    {
        return {};
    }

    if (trimmed.front() == '[')
    {
        return "list";
    }

    if (trimmed.front() == '{')
    {
        return "map";
    }

    return "scalar";
}

void AddIssue(
    std::vector<DataValidationIssue>& issues,
    const DataIssueSeverity severity,
    std::string code,
    std::string fieldPath,
    std::string message)
{
    issues.push_back(
        DataValidationIssue{severity, std::move(code), std::move(fieldPath), std::move(message)});
}

[[nodiscard]] bool HasErrorIssues(const std::vector<DataValidationIssue>& issues)
{
    return std::any_of(
        issues.begin(),
        issues.end(),
        [](const DataValidationIssue& issue)
        {
            return issue.severity == DataIssueSeverity::Error;
        });
}

[[nodiscard]] std::string BuildRegistryKey(
    const std::string_view recordId,
    const std::string_view sourcePath,
    const std::string_view schemaName)
{
    if (!recordId.empty())
    {
        return std::string(recordId);
    }

    if (!sourcePath.empty())
    {
        return std::string(schemaName) + ":" + std::string(sourcePath);
    }

    return std::string(schemaName) + ":<anonymous>";
}

[[nodiscard]] bool HasField(
    const std::vector<ParsedYamlField>& fields,
    const std::string_view fieldName)
{
    return std::any_of(
        fields.begin(),
        fields.end(),
        [fieldName](const ParsedYamlField& field)
        {
            return field.fieldName == fieldName;
        });
}

void UpdateNestedValueKind(
    ParsedYamlField& field,
    const std::string_view nextKind,
    std::vector<DataValidationIssue>& issues,
    const std::size_t lineNumber)
{
    if (field.valueKind.empty())
    {
        field.valueKind = std::string(nextKind);
        return;
    }

    if (field.valueKind != nextKind)
    {
        AddIssue(
            issues,
            DataIssueSeverity::Error,
            "yaml_mixed_value_kind",
            field.fieldName,
            "Line " + std::to_string(lineNumber) +
                " mixes YAML child shapes for field '" + field.fieldName + "'.");
    }
}

[[nodiscard]] bool ParseTopLevelYamlFields(
    const std::string& yamlText,
    std::vector<ParsedYamlField>& outFields,
    std::vector<DataValidationIssue>& issues)
{
    std::istringstream stream(yamlText);
    std::string line;
    bool parseSucceeded = true;
    bool sawTopLevelField = false;
    bool hasCurrentField = false;
    std::size_t currentFieldIndex = 0;
    std::size_t lineNumber = 0;

    while (std::getline(stream, line))
    {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        const std::string trimmed = TrimCopy(line);
        if (trimmed.empty() || trimmed.front() == '#')
        {
            continue;
        }

        const std::size_t indent = CountIndent(line);
        if (indent == 0)
        {
            hasCurrentField = false;

            if (trimmed.front() == '-')
            {
                AddIssue(
                    issues,
                    DataIssueSeverity::Error,
                    "yaml_root_not_mapping",
                    "$",
                    "Line " + std::to_string(lineNumber) +
                        " starts with a list item. The current loader contract expects a top-level mapping.");
                parseSucceeded = false;
                continue;
            }

            const std::size_t separator = trimmed.find(':');
            if (separator == std::string::npos)
            {
                AddIssue(
                    issues,
                    DataIssueSeverity::Error,
                    "yaml_invalid_mapping",
                    "$",
                    "Line " + std::to_string(lineNumber) +
                        " does not contain a top-level 'key: value' mapping entry.");
                parseSucceeded = false;
                continue;
            }

            const std::string fieldName = TrimCopy(trimmed.substr(0, separator));
            if (fieldName.empty())
            {
                AddIssue(
                    issues,
                    DataIssueSeverity::Error,
                    "yaml_missing_field_name",
                    "$",
                    "Line " + std::to_string(lineNumber) + " contains an empty top-level field name.");
                parseSucceeded = false;
                continue;
            }

            if (HasField(outFields, fieldName))
            {
                AddIssue(
                    issues,
                    DataIssueSeverity::Error,
                    "yaml_duplicate_field",
                    fieldName,
                    "Field '" + fieldName + "' appears more than once at the top level.");
                parseSucceeded = false;
                continue;
            }

            outFields.push_back(ParsedYamlField{fieldName, InferInlineValueKind(trimmed.substr(separator + 1))});
            currentFieldIndex = outFields.size() - 1;
            hasCurrentField = true;
            sawTopLevelField = true;
            continue;
        }

        if (!hasCurrentField)
        {
            AddIssue(
                issues,
                DataIssueSeverity::Error,
                "yaml_orphan_indentation",
                "$",
                "Line " + std::to_string(lineNumber) +
                    " is indented without a parent top-level field.");
            parseSucceeded = false;
            continue;
        }

        if (trimmed.front() == '-')
        {
            UpdateNestedValueKind(outFields[currentFieldIndex], "list", issues, lineNumber);
            continue;
        }

        UpdateNestedValueKind(outFields[currentFieldIndex], "map", issues, lineNumber);
    }

    if (!sawTopLevelField)
    {
        AddIssue(
            issues,
            DataIssueSeverity::Error,
            "yaml_empty_document",
            "$",
            "The YAML document is empty. The current loader contract expects top-level fields.");
        parseSucceeded = false;
    }

    for (auto& field : outFields)
    {
        if (field.valueKind.empty())
        {
            field.valueKind = "scalar";
        }
    }

    return parseSucceeded;
}

[[nodiscard]] std::size_t CountRecordsForSchema(
    const std::map<std::string, DataRegistryEntry>& records,
    const std::string_view schemaName,
    const bool trustedOnly)
{
    return static_cast<std::size_t>(std::count_if(
        records.begin(),
        records.end(),
        [schemaName, trustedOnly](const auto& entry)
        {
            const bool schemaMatches = entry.second.schemaName == schemaName;
            if (!schemaMatches)
            {
                return false;
            }

            return !trustedOnly || entry.second.trusted;
        }));
}
} // namespace

void DataService::Initialize()
{
    m_schemas.clear();
    m_records.clear();
    SHE_LOG_INFO("Data", "Data service initialized.");
}

void DataService::Shutdown()
{
    SHE_LOG_INFO("Data", "Data service shutdown complete.");
    m_records.clear();
    m_schemas.clear();
}

DataSchemaRegistrationResult DataService::RegisterSchema(DataSchemaContract schema)
{
    DataSchemaRegistrationResult result;
    result.schemaName = schema.schemaName;

    if (schema.schemaName.empty())
    {
        AddIssue(
            result.issues,
            DataIssueSeverity::Error,
            "schema_name_missing",
            "schema",
            "Schema registration requires a stable schemaName.");
    }

    if (schema.description.empty())
    {
        AddIssue(
            result.issues,
            DataIssueSeverity::Error,
            "schema_description_missing",
            "schema",
            "Schema '" + schema.schemaName + "' is missing a description.");
    }

    if (schema.owningSystem.empty())
    {
        AddIssue(
            result.issues,
            DataIssueSeverity::Error,
            "schema_owner_missing",
            "schema",
            "Schema '" + schema.schemaName + "' must declare an owningSystem.");
    }

    if (schema.fields.empty())
    {
        AddIssue(
            result.issues,
            DataIssueSeverity::Error,
            "schema_fields_missing",
            "schema",
            "Schema '" + schema.schemaName + "' must declare at least one field contract.");
    }

    std::map<std::string, bool> fieldNames;
    for (auto& field : schema.fields)
    {
        field.valueKind = NormalizeValueKind(field.valueKind);

        if (field.fieldName.empty())
        {
            AddIssue(
                result.issues,
                DataIssueSeverity::Error,
                "schema_field_name_missing",
                "schema.fields",
                "Schema '" + schema.schemaName + "' contains a field with an empty name.");
            continue;
        }

        if (field.valueKind != "scalar" && field.valueKind != "list" && field.valueKind != "map")
        {
            AddIssue(
                result.issues,
                DataIssueSeverity::Error,
                "schema_field_kind_invalid",
                field.fieldName,
                "Field '" + field.fieldName + "' uses unsupported valueKind '" + field.valueKind +
                    "'. Supported kinds are scalar, list, and map.");
            continue;
        }

        if (fieldNames.contains(field.fieldName))
        {
            AddIssue(
                result.issues,
                DataIssueSeverity::Error,
                "schema_field_duplicate",
                field.fieldName,
                "Field '" + field.fieldName + "' is declared more than once in schema '" + schema.schemaName + "'.");
            continue;
        }

        fieldNames.emplace(field.fieldName, true);
    }

    if (HasErrorIssues(result.issues))
    {
        SHE_LOG_WARNING("Data", "Rejected schema registration for '" + schema.schemaName + "'.");
        return result;
    }

    if (m_schemas.contains(schema.schemaName))
    {
        AddIssue(
            result.issues,
            DataIssueSeverity::Warning,
            "schema_replaced",
            schema.schemaName,
            "Schema '" + schema.schemaName + "' was replaced with a newer contract definition.");
    }

    result.accepted = true;
    result.schemaName = schema.schemaName;
    m_schemas.insert_or_assign(schema.schemaName, std::move(schema));
    return result;
}

DataLoadResult DataService::LoadRecord(const DataLoadRequest& request)
{
    DataLoadResult result;
    result.schemaName = request.schemaName;
    result.recordId = request.recordId;
    result.sourcePath = request.sourcePath;

    if (request.schemaName.empty())
    {
        AddIssue(
            result.issues,
            DataIssueSeverity::Error,
            "record_schema_missing",
            "schema",
            "A data load request must name the schema contract it wants to use.");
    }

    if (request.recordId.empty())
    {
        AddIssue(
            result.issues,
            DataIssueSeverity::Error,
            "record_id_missing",
            "recordId",
            "A data load request must provide a stable recordId for registry queries.");
    }

    if (request.sourcePath.empty())
    {
        AddIssue(
            result.issues,
            DataIssueSeverity::Warning,
            "record_source_missing",
            "sourcePath",
            "This data load request does not declare a sourcePath. Registry output will be less helpful.");
    }

    std::vector<ParsedYamlField> parsedFields;
    if (request.yamlText.empty())
    {
        AddIssue(
            result.issues,
            DataIssueSeverity::Error,
            "yaml_payload_missing",
            "$",
            "A data load request must provide YAML text to validate.");
    }
    else
    {
        result.parsed = ParseTopLevelYamlFields(request.yamlText, parsedFields, result.issues);
    }

    for (const auto& field : parsedFields)
    {
        result.discoveredFields.push_back(field.fieldName);
    }

    const auto schemaIterator = m_schemas.find(request.schemaName);
    if (schemaIterator == m_schemas.end())
    {
        AddIssue(
            result.issues,
            DataIssueSeverity::Error,
            "schema_not_registered",
            "schema",
            "Schema '" + request.schemaName + "' is not registered in the data service.");
    }
    else
    {
        const auto& schema = schemaIterator->second;
        std::map<std::string, DataSchemaFieldContract> expectedFields;
        for (const auto& field : schema.fields)
        {
            expectedFields.emplace(field.fieldName, field);
        }

        for (const auto& expectedField : schema.fields)
        {
            const bool isPresent = std::any_of(
                parsedFields.begin(),
                parsedFields.end(),
                [&expectedField](const ParsedYamlField& parsedField)
                {
                    return parsedField.fieldName == expectedField.fieldName;
                });

            if (expectedField.required && !isPresent)
            {
                AddIssue(
                    result.issues,
                    DataIssueSeverity::Error,
                    "required_field_missing",
                    expectedField.fieldName,
                    "Required field '" + expectedField.fieldName + "' is missing from record '" +
                        request.recordId + "'.");
            }
        }

        for (const auto& parsedField : parsedFields)
        {
            const auto expectedFieldIterator = expectedFields.find(parsedField.fieldName);
            if (expectedFieldIterator == expectedFields.end())
            {
                AddIssue(
                    result.issues,
                    DataIssueSeverity::Error,
                    "field_not_declared",
                    parsedField.fieldName,
                    "Field '" + parsedField.fieldName + "' is not declared by schema '" + request.schemaName + "'.");
                continue;
            }

            if (expectedFieldIterator->second.valueKind != parsedField.valueKind)
            {
                AddIssue(
                    result.issues,
                    DataIssueSeverity::Error,
                    "field_kind_mismatch",
                    parsedField.fieldName,
                    "Field '" + parsedField.fieldName + "' expected valueKind '" +
                        expectedFieldIterator->second.valueKind + "' but YAML provided '" +
                        parsedField.valueKind + "'.");
            }
        }
    }

    result.valid = result.parsed && !HasErrorIssues(result.issues);

    DataRegistryEntry entry;
    entry.recordId = result.recordId;
    entry.schemaName = result.schemaName;
    entry.sourcePath = result.sourcePath;
    entry.trusted = result.valid;
    entry.discoveredFields = result.discoveredFields;
    entry.issueCount = result.issues.size();
    m_records.insert_or_assign(
        BuildRegistryKey(result.recordId, result.sourcePath, result.schemaName),
        std::move(entry));

    return result;
}

bool DataService::HasSchema(const std::string_view schemaName) const
{
    return m_schemas.contains(std::string(schemaName));
}

bool DataService::HasTrustedRecord(const std::string_view recordId) const
{
    const auto iterator = m_records.find(std::string(recordId));
    return iterator != m_records.end() && iterator->second.trusted;
}

std::size_t DataService::GetSchemaCount() const
{
    return m_schemas.size();
}

std::size_t DataService::GetRecordCount() const
{
    return m_records.size();
}

std::size_t DataService::GetTrustedRecordCount() const
{
    return static_cast<std::size_t>(std::count_if(
        m_records.begin(),
        m_records.end(),
        [](const auto& entry)
        {
            return entry.second.trusted;
        }));
}

std::vector<DataSchemaContract> DataService::ListSchemas() const
{
    std::vector<DataSchemaContract> schemas;
    schemas.reserve(m_schemas.size());
    for (const auto& [name, schema] : m_schemas)
    {
        schemas.push_back(schema);
    }

    return schemas;
}

std::vector<DataRegistryEntry> DataService::ListRecords() const
{
    std::vector<DataRegistryEntry> records;
    records.reserve(m_records.size());
    for (const auto& [recordKey, record] : m_records)
    {
        records.push_back(record);
    }

    return records;
}

std::string DataService::DescribeSchemas() const
{
    if (m_schemas.empty())
    {
        return "(no schemas registered)\n";
    }

    std::ostringstream stream;
    for (const auto& [name, schema] : m_schemas)
    {
        const std::size_t totalRecords = CountRecordsForSchema(m_records, name, false);
        const std::size_t trustedRecords = CountRecordsForSchema(m_records, name, true);
        stream << "- " << name << ": " << schema.description
               << " | owner=" << schema.owningSystem
               << " | fields=" << schema.fields.size()
               << " | trusted_records=" << trustedRecords << "/" << totalRecords << '\n';

        for (const auto& field : schema.fields)
        {
            stream << "  - " << field.fieldName
                   << " (" << field.valueKind << ", " << (field.required ? "required" : "optional") << ")";
            if (!field.description.empty())
            {
                stream << ": " << field.description;
            }
            stream << '\n';
        }
    }

    return stream.str();
}

std::string DataService::DescribeRegistry() const
{
    if (m_records.empty())
    {
        return "(no data records loaded)\n";
    }

    std::ostringstream stream;
    for (const auto& [recordKey, record] : m_records)
    {
        stream << "- " << recordKey
               << ": schema=" << record.schemaName
               << " | trusted=" << (record.trusted ? "true" : "false")
               << " | source=" << (record.sourcePath.empty() ? "<memory>" : record.sourcePath)
               << " | fields=";

        for (std::size_t index = 0; index < record.discoveredFields.size(); ++index)
        {
            stream << record.discoveredFields[index];
            if (index + 1 < record.discoveredFields.size())
            {
                stream << ", ";
            }
        }

        stream << " | issues=" << record.issueCount << '\n';
    }

    return stream.str();
}
} // namespace she
