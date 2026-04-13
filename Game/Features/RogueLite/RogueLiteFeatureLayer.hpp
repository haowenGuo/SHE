#pragma once

#include "RogueLiteGame.hpp"

#include "SHE/Core/Layer.hpp"

#include <optional>
#include <string>

namespace she
{
class RogueLiteFeatureLayer final : public Layer
{
public:
    RogueLiteFeatureLayer();

    void OnAttach(RuntimeServices& services) override;
    void OnUpdate(const TickContext& context) override;
    void OnRender(const TickContext& context) override;
    void OnUi(const TickContext& context) override;
    void OnDetach(RuntimeServices& services) override;

    [[nodiscard]] RogueLiteSnapshot BuildSnapshot() const;
    [[nodiscard]] const std::string& GetLatestDebugReport() const;
    [[nodiscard]] const std::string& GetLatestAuthoringContext() const;
    [[nodiscard]] const std::string& GetLatestGameplayDigest() const;
    [[nodiscard]] const std::optional<RenderFrameSnapshot>& GetLastCompletedFrame() const;

private:
    void RegisterFeatureContracts(RuntimeServices& services);
    void RegisterDataContracts(RuntimeServices& services);
    void RegisterVisualAssets(RuntimeServices& services);
    void RegisterGameplayContracts(RuntimeServices& services);
    void SubscribeToGameplay(RuntimeServices& services);
    void QueueGameplayCommand(std::string_view name, std::string payload = {});
    void QueueInputCommands(const TickContext& context);
    void HandleGameplayAction(const GameplayEvent& event);
    void PublishActionEvents(const RogueLiteAction action, const RogueLiteActionResult& result, const RogueLiteSnapshot& snapshot);
    void PublishUpdateEvents(const RogueLiteUpdateResult& result, const RogueLiteSnapshot& before, const RogueLiteSnapshot& after);
    void SubmitSprite(
        const TickContext& context,
        std::string_view entityName,
        std::string_view materialName,
        Vector2 position,
        Vector2 size,
        const Color& tint,
        int sortKey) const;
    void SubmitHudBar(
        const TickContext& context,
        Vector2 center,
        Vector2 size,
        float ratio,
        const Color& fillTint,
        const Color& backTint,
        int sortKey) const;
    void RenderRoom(const TickContext& context, const RogueLiteSnapshot& snapshot) const;
    void RenderHud(const TickContext& context, const RogueLiteSnapshot& snapshot) const;

    RuntimeServices* m_services = nullptr;
    RogueLiteGame m_game;
    EntityId m_cameraEntityId = kInvalidEntityId;
    std::size_t m_actionSubscriptionId = 0;
    bool m_loggedControls = false;
    std::string m_latestDebugReport;
    std::string m_latestAuthoringContext;
    std::string m_latestGameplayDigest;
    std::optional<RenderFrameSnapshot> m_lastCompletedFrame;
};
} // namespace she
