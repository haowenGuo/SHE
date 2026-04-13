#include "SHE/AI/AuthoringAiService.hpp"

#include "SHE/Core/Logger.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace she
{
namespace
{
std::size_t CountNonEmptyLines(const std::string_view text)
{
    if (text.empty())
    {
        return 0;
    }

    std::size_t count = 0;
    std::istringstream stream(std::string{text});
    std::string line;
    while (std::getline(stream, line))
    {
        if (!line.empty())
        {
            ++count;
        }
    }

    return count;
}

void AppendIndentedBlock(std::ostringstream& stream, const std::string_view text)
{
    if (text.empty())
    {
        stream << "  <empty>\n";
        return;
    }

    std::istringstream block(std::string{text});
    std::string line;
    while (std::getline(block, line))
    {
        stream << "  " << line << '\n';
    }
}

void AppendContentSection(std::ostringstream& stream, const std::string_view sectionName, const std::string& content)
{
    stream << '\n' << '[' << sectionName << "]\n";
    stream << "line_count: " << CountNonEmptyLines(content) << '\n';
    stream << "content:\n";
    AppendIndentedBlock(stream, content);
}

std::string JoinFields(const std::vector<std::string>& fields)
{
    std::ostringstream stream;
    if (fields.empty())
    {
        stream << "<none>";
        return stream.str();
    }

    for (std::size_t index = 0; index < fields.size(); ++index)
    {
        stream << fields[index];
        if (index + 1 < fields.size())
        {
            stream << ", ";
        }
    }

    return stream.str();
}

std::string JoinAssetProperties(const std::vector<AssetMetadataProperty>& properties)
{
    std::ostringstream stream;
    if (properties.empty())
    {
        stream << "<none>";
        return stream.str();
    }

    for (std::size_t index = 0; index < properties.size(); ++index)
    {
        stream << properties[index].key << '=' << properties[index].value;
        if (index + 1 < properties.size())
        {
            stream << ", ";
        }
    }

    return stream.str();
}

std::string BuildAssetCatalog(const IAssetService& assets)
{
    const auto entries = assets.ListAssets();
    const auto loaders = assets.ListLoaders();
    const std::string describedRegistry = assets.DescribeRegistry();
    const std::string describedLoaders = assets.DescribeLoaders();

    std::size_t liveHandleCount = 0;
    for (const auto& entry : entries)
    {
        liveHandleCount += entry.liveHandleCount;
    }

    std::ostringstream stream;
    stream << "asset_registry_version: 1\n";
    stream << "asset_count: " << entries.size() << '\n';
    stream << "loader_count: " << loaders.size() << '\n';
    stream << "live_handle_count: " << liveHandleCount << '\n';

    for (std::size_t index = 0; index < entries.size(); ++index)
    {
        const auto& entry = entries[index];
        stream << "\n[asset_" << index << "]\n";
        stream << "asset_id: " << entry.assetId << '\n';
        stream << "logical_name: " << entry.logicalName << '\n';
        stream << "source_path: " << entry.sourcePath << '\n';
        stream << "asset_type: " << entry.assetType << '\n';
        stream << "declared_loader_key: " << (entry.declaredLoaderKey.empty() ? "<auto>" : entry.declaredLoaderKey) << '\n';
        stream << "resolved_loader_key: " << (entry.resolvedLoaderKey.empty() ? "<unresolved>" : entry.resolvedLoaderKey) << '\n';
        stream << "owning_system: " << entry.owningSystem << '\n';
        stream << "source_record_id: " << (entry.sourceRecordId.empty() ? "<none>" : entry.sourceRecordId) << '\n';
        stream << "tags: " << JoinFields(entry.tags) << '\n';
        stream << "properties: " << JoinAssetProperties(entry.properties) << '\n';
        stream << "live_handle_count: " << entry.liveHandleCount << '\n';
    }

    for (std::size_t index = 0; index < loaders.size(); ++index)
    {
        const auto& loader = loaders[index];
        stream << "\n[loader_" << index << "]\n";
        stream << "loader_key: " << loader.loaderKey << '\n';
        stream << "asset_type: " << loader.assetType << '\n';
        stream << "supported_extensions: " << JoinFields(loader.supportedExtensions) << '\n';
        stream << "description: " << (loader.description.empty() ? "<none>" : loader.description) << '\n';
    }

    stream << "\n[registry_report]\n";
    stream << "line_count: " << CountNonEmptyLines(describedRegistry) << '\n';
    stream << "content:\n";
    AppendIndentedBlock(stream, describedRegistry);

    stream << "\n[loader_report]\n";
    stream << "line_count: " << CountNonEmptyLines(describedLoaders) << '\n';
    stream << "content:\n";
    AppendIndentedBlock(stream, describedLoaders);

    return stream.str();
}

std::string BuildSchemaCatalog(
    const std::vector<DataSchemaContract>& schemas,
    const std::string_view describedSchemas)
{
    std::ostringstream stream;
    stream << "schema_catalog_version: 1\n";
    stream << "schema_count: " << schemas.size() << '\n';

    for (std::size_t index = 0; index < schemas.size(); ++index)
    {
        const auto& schema = schemas[index];
        stream << "\n[schema_" << index << "]\n";
        stream << "schema_name: " << schema.schemaName << '\n';
        stream << "description: " << schema.description << '\n';
        stream << "owning_system: " << schema.owningSystem << '\n';
        stream << "field_count: " << schema.fields.size() << '\n';

        if (schema.fields.empty())
        {
            stream << "fields: <none>\n";
        }
        else
        {
            stream << "fields:\n";
            for (const auto& field : schema.fields)
            {
                stream << "- name=" << field.fieldName
                       << ", kind=" << field.valueKind
                       << ", required=" << (field.required ? "true" : "false");
                if (!field.description.empty())
                {
                    stream << ", description=" << field.description;
                }
                stream << '\n';
            }
        }
    }

    stream << "\n[schema_report]\n";
    stream << "line_count: " << CountNonEmptyLines(describedSchemas) << '\n';
    stream << "content:\n";
    AppendIndentedBlock(stream, describedSchemas);

    return stream.str();
}

std::string BuildDataRegistryCatalog(const IDataService& data)
{
    const auto records = data.ListRecords();
    const std::size_t trustedRecordCount = data.GetTrustedRecordCount();
    const std::string describedRegistry = data.DescribeRegistry();

    std::ostringstream stream;
    stream << "data_registry_version: 1\n";
    stream << "schema_count: " << data.GetSchemaCount() << '\n';
    stream << "record_count: " << records.size() << '\n';
    stream << "trusted_record_count: " << trustedRecordCount << '\n';
    stream << "untrusted_record_count: " << (records.size() - trustedRecordCount) << '\n';

    for (std::size_t index = 0; index < records.size(); ++index)
    {
        const auto& record = records[index];
        stream << "\n[record_" << index << "]\n";
        stream << "record_id: " << record.recordId << '\n';
        stream << "schema_name: " << record.schemaName << '\n';
        stream << "source_path: " << record.sourcePath << '\n';
        stream << "trusted: " << (record.trusted ? "true" : "false") << '\n';
        stream << "discovered_fields: " << JoinFields(record.discoveredFields) << '\n';
        stream << "issue_count: " << record.issueCount << '\n';
    }

    stream << "\n[registry_report]\n";
    stream << "line_count: " << CountNonEmptyLines(describedRegistry) << '\n';
    stream << "content:\n";
    AppendIndentedBlock(stream, describedRegistry);
    return stream.str();
}
} // namespace

AuthoringAiService::AuthoringAiService(
    std::shared_ptr<IReflectionService> reflection,
    std::shared_ptr<IDataService> data,
    std::shared_ptr<IGameplayService> gameplay,
    std::shared_ptr<IScriptingService> scripting,
    std::shared_ptr<ISceneService> scene,
    std::shared_ptr<IAssetService> assets,
    std::shared_ptr<IDiagnosticsService> diagnostics)
    : m_reflection(std::move(reflection))
    , m_data(std::move(data))
    , m_gameplay(std::move(gameplay))
    , m_scripting(std::move(scripting))
    , m_scene(std::move(scene))
    , m_assets(std::move(assets))
    , m_diagnostics(std::move(diagnostics))
{
}

void AuthoringAiService::Initialize()
{
    m_cachedContext.clear();
    m_contextVersion = 0;
    SHE_LOG_INFO("AI", "Authoring AI service initialized.");
}

void AuthoringAiService::Shutdown()
{
    SHE_LOG_INFO("AI", "Authoring AI service shutdown complete.");
}

void AuthoringAiService::SetAuthoringIntent(std::string intent)
{
    m_authoringIntent = std::move(intent);
}

void AuthoringAiService::RefreshContext(const FrameIndex frameIndex)
{
    const std::size_t nextContextVersion = m_contextVersion + 1;
    const std::string reflectionCatalog = m_reflection->BuildTypeCatalog();
    const std::string assetCatalog = BuildAssetCatalog(*m_assets);
    const auto schemas = m_data->ListSchemas();
    const std::string schemaCatalog = BuildSchemaCatalog(schemas, m_data->DescribeSchemas());
    const std::string dataRegistry = BuildDataRegistryCatalog(*m_data);
    const std::string gameplayDigest = m_gameplay->BuildGameplayDigest();
    const std::string scriptCatalog = m_scripting->BuildScriptCatalog();
    const std::string latestFrameReport = m_diagnostics->BuildLatestReport();

    std::ostringstream stream;
    stream << "authoring_context_contract_version: 1\n";
    stream << "context_version: " << nextContextVersion << '\n';
    stream << "frame_index: " << frameIndex << '\n';

    stream << "\n[project]\n";
    stream << "intent: " << m_authoringIntent << '\n';
    stream << "context_is_read_only: true\n";
    stream << "runtime_mutation_allowed: false\n";

    stream << "\n[runtime_state]\n";
    stream << "active_scene: " << m_scene->GetActiveSceneName() << '\n';
    stream << "entity_count: " << m_scene->GetEntityCount() << '\n';
    stream << "asset_count: " << m_assets->GetAssetCount() << '\n';

    stream << "\n[module_counts]\n";
    stream << "registered_types: " << m_reflection->GetRegisteredTypeCount() << '\n';
    stream << "registered_features: " << m_reflection->GetRegisteredFeatureCount() << '\n';
    stream << "registered_schemas: " << m_data->GetSchemaCount() << '\n';
    stream << "data_records: " << m_data->GetRecordCount() << '\n';
    stream << "trusted_data_records: " << m_data->GetTrustedRecordCount() << '\n';
    stream << "registered_script_bindings: " << m_scripting->GetBindingCount() << '\n';
    stream << "registered_script_modules: " << m_scripting->GetModuleCount() << '\n';
    stream << "loaded_script_modules: " << m_scripting->GetLoadedModuleCount() << '\n';
    stream << "captured_diagnostic_frames: " << m_diagnostics->GetCapturedFrameCount() << '\n';

    AppendContentSection(stream, "reflection_catalog", reflectionCatalog);
    AppendContentSection(stream, "asset_registry", assetCatalog);
    AppendContentSection(stream, "schema_catalog", schemaCatalog);
    AppendContentSection(stream, "data_registry", dataRegistry);
    AppendContentSection(stream, "gameplay_state", gameplayDigest);
    AppendContentSection(stream, "script_catalog", scriptCatalog);
    AppendContentSection(stream, "latest_frame_report", latestFrameReport);

    m_cachedContext = stream.str();
    m_contextVersion = nextContextVersion;
}

std::size_t AuthoringAiService::GetContextVersion() const
{
    return m_contextVersion;
}

std::string AuthoringAiService::ExportAuthoringContext() const
{
    return m_cachedContext;
}
} // namespace she
