#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <memory>
#include <string>

namespace she
{
class DebugUiService final : public IUiService
{
public:
    DebugUiService(
        std::shared_ptr<IWindowService> window,
        std::shared_ptr<IAssetService> assets,
        std::shared_ptr<ISceneService> scene,
        std::shared_ptr<IDataService> data,
        std::shared_ptr<IGameplayService> gameplay,
        std::shared_ptr<IRendererService> renderer,
        std::shared_ptr<IPhysicsService> physics,
        std::shared_ptr<IDiagnosticsService> diagnostics,
        std::shared_ptr<IAIService> ai);

    void Initialize() override;
    void Shutdown() override;
    void BeginFrame(FrameIndex frameIndex) override;
    void EndFrame() override;
    [[nodiscard]] std::string BuildLatestDebugReport() const override;

private:
    void PublishDebugReport(FrameIndex frameIndex);

    std::shared_ptr<IWindowService> m_window;
    std::shared_ptr<IAssetService> m_assets;
    std::shared_ptr<ISceneService> m_scene;
    std::shared_ptr<IDataService> m_data;
    std::shared_ptr<IGameplayService> m_gameplay;
    std::shared_ptr<IRendererService> m_renderer;
    std::shared_ptr<IPhysicsService> m_physics;
    std::shared_ptr<IDiagnosticsService> m_diagnostics;
    std::shared_ptr<IAIService> m_ai;
    FrameIndex m_lastFrameIndex = 0;
    std::string m_latestDebugReport;
    bool m_initialized = false;
};
} // namespace she
