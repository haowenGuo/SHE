#include "SHE/AI/AuthoringAiService.hpp"

#include "SHE/Core/Logger.hpp"

#include <sstream>
#include <utility>

namespace she
{
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
    std::ostringstream stream;
    stream << "intent: " << m_authoringIntent << '\n';
    stream << "frame: " << frameIndex << '\n';
    stream << "active_scene: " << m_scene->GetActiveSceneName() << '\n';
    stream << "entity_count: " << m_scene->GetEntityCount() << '\n';
    stream << "asset_count: " << m_assets->GetAssetCount() << '\n';
    stream << "type_count: " << m_reflection->GetRegisteredTypeCount() << '\n';
    stream << "feature_count: " << m_reflection->GetRegisteredFeatureCount() << '\n';
    stream << "schema_count: " << m_data->GetSchemaCount() << '\n';
    stream << "script_module_count: " << m_scripting->GetModuleCount() << '\n';
    stream << "diagnostic_frames: " << m_diagnostics->GetCapturedFrameCount() << '\n';
    stream << "\n[type_catalog]\n" << m_reflection->BuildTypeCatalog();
    stream << "\n[schema_catalog]\n" << m_data->DescribeSchemas();
    stream << "\n[gameplay_digest]\n" << m_gameplay->BuildGameplayDigest();
    stream << "\n[script_catalog]\n" << m_scripting->BuildScriptCatalog();
    stream << "\n[latest_frame_report]\n" << m_diagnostics->BuildLatestReport();

    m_cachedContext = stream.str();
    ++m_contextVersion;
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
