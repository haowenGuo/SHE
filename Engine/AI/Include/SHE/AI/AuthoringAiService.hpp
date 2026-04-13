#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <memory>
#include <string>

namespace she
{
// AuthoringAiService is not an online LLM runtime. It is the engine-facing
// context exporter that packages the current project's known contracts into a
// concise, read-only authoring snapshot for Codex or future AI tooling.
class AuthoringAiService final : public IAIService
{
public:
    AuthoringAiService(
        std::shared_ptr<IReflectionService> reflection,
        std::shared_ptr<IDataService> data,
        std::shared_ptr<IGameplayService> gameplay,
        std::shared_ptr<IScriptingService> scripting,
        std::shared_ptr<ISceneService> scene,
        std::shared_ptr<IAssetService> assets,
        std::shared_ptr<IDiagnosticsService> diagnostics);

    void Initialize() override;
    void Shutdown() override;
    void SetAuthoringIntent(std::string intent) override;
    void RefreshContext(FrameIndex frameIndex) override;
    [[nodiscard]] std::size_t GetContextVersion() const override;
    [[nodiscard]] std::string ExportAuthoringContext() const override;

private:
    std::shared_ptr<IReflectionService> m_reflection;
    std::shared_ptr<IDataService> m_data;
    std::shared_ptr<IGameplayService> m_gameplay;
    std::shared_ptr<IScriptingService> m_scripting;
    std::shared_ptr<ISceneService> m_scene;
    std::shared_ptr<IAssetService> m_assets;
    std::shared_ptr<IDiagnosticsService> m_diagnostics;
    std::string m_authoringIntent = "Build gameplay features through engine contracts and data schemas.";
    // The cached export is rebuilt once per frame so tooling can inspect a
    // stable snapshot without asking subsystems to re-run expensive formatting.
    std::string m_cachedContext;
    std::size_t m_contextVersion = 0;
};
} // namespace she

