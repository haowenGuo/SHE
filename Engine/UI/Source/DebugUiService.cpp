#include "SHE/UI/DebugUiService.hpp"

#include "SHE/Core/Logger.hpp"

#include <algorithm>
#include <sstream>
#include <string_view>
#include <utility>

namespace she
{
namespace
{
[[nodiscard]] std::string BoolToString(const bool value)
{
    return value ? "true" : "false";
}

[[nodiscard]] std::size_t CountNonEmptyLines(const std::string_view text)
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

[[nodiscard]] std::string BuildPreview(const std::string_view text, const std::size_t maxLines)
{
    if (text.empty() || maxLines == 0)
    {
        return {};
    }

    std::ostringstream stream;
    std::istringstream block(std::string{text});
    std::string line;
    std::size_t linesWritten = 0;
    while (linesWritten < maxLines && std::getline(block, line))
    {
        if (line.empty())
        {
            continue;
        }

        stream << line << '\n';
        ++linesWritten;
    }

    return stream.str();
}

[[nodiscard]] std::string BuildEntityLine(const SceneEntityView& entity)
{
    std::ostringstream stream;
    stream << "- id=" << entity.entityId << ", name=" << entity.entityName << ", position="
           << entity.transform.position.x << ',' << entity.transform.position.y << ", rotation="
           << entity.transform.rotationDegrees << ", scale=" << entity.transform.scale.x << ','
           << entity.transform.scale.y;
    return stream.str();
}

void AppendLatestFrame(std::ostringstream& stream, const std::optional<RenderFrameSnapshot>& frame)
{
    stream << "[renderer]\n";
    if (!frame.has_value())
    {
        stream << "has_last_completed_frame: false\n";
        return;
    }

    stream << "has_last_completed_frame: true\n";
    stream << "last_frame_index: " << frame->frameIndex << '\n';
    stream << "backend_name: " << frame->backendName << '\n';
    stream << "scene_name: " << frame->sceneName << '\n';
    stream << "scene_entity_count: " << frame->sceneEntityCount << '\n';
    stream << "surface_size: " << frame->surfaceWidth << 'x' << frame->surfaceHeight << '\n';
    stream << "camera_name: " << (frame->camera.has_value() ? frame->camera->cameraName : "<none>") << '\n';
    stream << "sprite_count: " << frame->sprites.size() << '\n';
    stream << "visible_pixel_sample_count: " << frame->visiblePixelSampleCount << '\n';
    stream << "presented_to_surface: " << BoolToString(frame->presentedToSurface) << '\n';

    if (!frame->sprites.empty())
    {
        stream << "sprites:\n";
        for (const RenderedSprite2D& sprite : frame->sprites)
        {
            stream << "- entity_id=" << sprite.entityId << ", entity_name=" << sprite.entityName
                   << ", material=" << sprite.material.materialName << ", texture="
                   << sprite.material.textureAssetName << ", position=" << sprite.position.x << ','
                   << sprite.position.y << ", sort_key=" << sprite.sortKey << '\n';
        }
    }
}
} // namespace

DebugUiService::DebugUiService(
    std::shared_ptr<IWindowService> window,
    std::shared_ptr<IAssetService> assets,
    std::shared_ptr<ISceneService> scene,
    std::shared_ptr<IDataService> data,
    std::shared_ptr<IGameplayService> gameplay,
    std::shared_ptr<IRendererService> renderer,
    std::shared_ptr<IPhysicsService> physics,
    std::shared_ptr<IDiagnosticsService> diagnostics,
    std::shared_ptr<IAIService> ai)
    : m_window(std::move(window))
    , m_assets(std::move(assets))
    , m_scene(std::move(scene))
    , m_data(std::move(data))
    , m_gameplay(std::move(gameplay))
    , m_renderer(std::move(renderer))
    , m_physics(std::move(physics))
    , m_diagnostics(std::move(diagnostics))
    , m_ai(std::move(ai))
{
}

void DebugUiService::Initialize()
{
    m_lastFrameIndex = 0;
    m_latestDebugReport =
        "ui_debug_report_version: 1\nstatus: initialized\nnote: runtime panels become meaningful after the first completed frame.\n";
    m_initialized = true;
    SHE_LOG_INFO("UI", "Debug UI service initialized.");
}

void DebugUiService::Shutdown()
{
    SHE_LOG_INFO("UI", "Debug UI service shutdown complete.");
    m_latestDebugReport = "ui_debug_report_version: 1\nstatus: shutdown\n";
    m_initialized = false;
}

void DebugUiService::BeginFrame(const FrameIndex frameIndex)
{
    if (!m_initialized)
    {
        return;
    }

    m_lastFrameIndex = frameIndex;
    PublishDebugReport(frameIndex);
}

void DebugUiService::EndFrame()
{
}

std::string DebugUiService::BuildLatestDebugReport() const
{
    return m_latestDebugReport;
}

void DebugUiService::PublishDebugReport(const FrameIndex frameIndex)
{
    if (m_window == nullptr || m_assets == nullptr || m_scene == nullptr || m_data == nullptr || m_gameplay == nullptr ||
        m_renderer == nullptr || m_physics == nullptr || m_diagnostics == nullptr || m_ai == nullptr)
    {
        m_latestDebugReport = "ui_debug_report_version: 1\nstatus: incomplete_runtime\n";
        return;
    }

    const WindowState windowState = m_window->GetWindowState();
    const PointerState pointerState = m_window->GetPointerState();
    const RendererBackendInfo backendInfo = m_renderer->GetBackendInfo();
    const auto latestFrame = m_renderer->GetLastCompletedFrame();
    const std::vector<SceneEntityView> entities = m_scene->ListEntities();
    const std::string physicsSummary = m_physics->BuildDebugSummary();
    const std::string diagnosticsReport = m_diagnostics->BuildLatestReport();
    const std::string authoringContext = m_ai->ExportAuthoringContext();
    const std::string authoringPreview = BuildPreview(authoringContext, 14);

    std::ostringstream stream;
    stream << "ui_debug_report_version: 1\n";
    stream << "status: ready\n";
    stream << "frame_index: " << frameIndex << '\n';
    stream << "note: renderer, diagnostics, and authoring context sections describe the latest completed frame.\n\n";

    stream << "[runtime_overview]\n";
    stream << "active_scene: " << m_scene->GetActiveSceneName() << '\n';
    stream << "entity_count: " << entities.size() << '\n';
    stream << "asset_count: " << m_assets->GetAssetCount() << '\n';
    stream << "schema_count: " << m_data->GetSchemaCount() << '\n';
    stream << "record_count: " << m_data->GetRecordCount() << '\n';
    stream << "trusted_record_count: " << m_data->GetTrustedRecordCount() << '\n';
    stream << "pending_command_count: " << m_gameplay->GetPendingCommandCount() << '\n';
    stream << "timer_count: " << m_gameplay->GetTimerCount() << '\n';
    stream << "captured_diagnostic_frames: " << m_diagnostics->GetCapturedFrameCount() << '\n';
    stream << "authoring_context_version: " << m_ai->GetContextVersion() << '\n';
    stream << '\n';

    stream << "[window]\n";
    stream << "pump_count: " << m_window->GetPumpCount() << '\n';
    stream << "window_id: " << windowState.windowId << '\n';
    stream << "window_size: " << windowState.width << 'x' << windowState.height << '\n';
    stream << "has_keyboard_focus: " << BoolToString(windowState.hasKeyboardFocus) << '\n';
    stream << "has_mouse_focus: " << BoolToString(windowState.hasMouseFocus) << '\n';
    stream << "minimized: " << BoolToString(windowState.minimized) << '\n';
    stream << "close_requested: " << BoolToString(windowState.closeRequested) << '\n';
    stream << "pointer_position: " << pointerState.x << ',' << pointerState.y << '\n';
    stream << "pointer_delta: " << pointerState.deltaX << ',' << pointerState.deltaY << '\n';
    stream << "pointer_wheel: " << pointerState.wheelX << ',' << pointerState.wheelY << '\n';
    stream << '\n';

    stream << "[scene]\n";
    stream << "entity_count: " << entities.size() << '\n';
    if (entities.empty())
    {
        stream << "entities: <none>\n\n";
    }
    else
    {
        stream << "entities:\n";
        for (const SceneEntityView& entity : entities)
        {
            stream << BuildEntityLine(entity) << '\n';
        }
        stream << '\n';
    }

    stream << "[renderer_backend]\n";
    stream << "backend_name: " << backendInfo.backendName << '\n';
    stream << "default_surface_size: " << backendInfo.defaultSurfaceWidth << 'x' << backendInfo.defaultSurfaceHeight << '\n';
    stream << "supports_material_lookup: " << BoolToString(backendInfo.supportsMaterialLookup) << '\n';
    stream << "supports_frame_capture: " << BoolToString(backendInfo.supportsFrameCapture) << '\n';
    stream << "frame_in_flight: " << BoolToString(m_renderer->HasFrameInFlight()) << '\n\n';

    AppendLatestFrame(stream, latestFrame);
    stream << '\n';

    stream << "[physics]\n";
    stream << "line_count: " << CountNonEmptyLines(physicsSummary) << '\n';
    stream << "content:\n";
    AppendIndentedBlock(stream, physicsSummary);
    stream << '\n';

    stream << "[gameplay]\n";
    const std::string gameplayDigest = m_gameplay->BuildGameplayDigest();
    stream << "line_count: " << CountNonEmptyLines(gameplayDigest) << '\n';
    stream << "content:\n";
    AppendIndentedBlock(stream, gameplayDigest);
    stream << '\n';

    stream << "[diagnostics]\n";
    stream << "line_count: " << CountNonEmptyLines(diagnosticsReport) << '\n';
    stream << "content:\n";
    AppendIndentedBlock(stream, diagnosticsReport);
    stream << '\n';

    stream << "[authoring_context]\n";
    stream << "context_version: " << m_ai->GetContextVersion() << '\n';
    stream << "line_count: " << CountNonEmptyLines(authoringContext) << '\n';
    stream << "preview:\n";
    AppendIndentedBlock(stream, authoringPreview);

    m_latestDebugReport = stream.str();
}
} // namespace she
