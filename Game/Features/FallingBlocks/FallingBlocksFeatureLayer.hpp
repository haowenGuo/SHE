#pragma once

#include "FallingBlocksGame.hpp"

#include "SHE/Core/Layer.hpp"

#include <optional>
#include <string>

namespace she
{
class FallingBlocksFeatureLayer final : public Layer
{
public:
    FallingBlocksFeatureLayer();

    void OnAttach(RuntimeServices& services) override;
    void OnUpdate(const TickContext& context) override;
    void OnRender(const TickContext& context) override;
    void OnUi(const TickContext& context) override;
    void OnDetach(RuntimeServices& services) override;

    [[nodiscard]] FallingBlocksSnapshot BuildSnapshot() const;
    [[nodiscard]] const std::string& GetLatestDebugReport() const;
    [[nodiscard]] const std::string& GetLatestAuthoringContext() const;
    [[nodiscard]] const std::string& GetLatestGameplayDigest() const;
    [[nodiscard]] const std::optional<RenderFrameSnapshot>& GetLastCompletedFrame() const;

private:
    void RegisterFeatureContracts(RuntimeServices& services);
    void RegisterVisualAssets(RuntimeServices& services);
    void RegisterGameplayContracts(RuntimeServices& services);
    void SubscribeToGameplay(RuntimeServices& services);
    void ResetInputRepeatState();
    void QueueInputCommands(const TickContext& context);
    void QueueGameplayCommand(std::string_view commandName, std::string payload = {});
    void HandleGameplayAction(const GameplayEvent& event);
    void PublishActionOutcome(
        FallingBlocksAction action,
        const FallingBlocksActionResult& result,
        std::string_view source);
    void SubmitSprite(
        const TickContext& context,
        std::string_view entityName,
        std::string_view materialName,
        const Vector2& position,
        const Vector2& size,
        const Color& tint,
        int sortKey) const;
    void SubmitBoard(const TickContext& context, const FallingBlocksSnapshot& snapshot) const;
    void SubmitHud(const TickContext& context, const FallingBlocksSnapshot& snapshot) const;
    void SubmitSevenSegmentNumber(
        const TickContext& context,
        int value,
        int digitCount,
        const Vector2& anchor,
        float scale,
        const Color& litTint,
        const Color& unlitTint,
        int sortKey) const;
    void SubmitSevenSegmentDigit(
        const TickContext& context,
        int digit,
        const Vector2& center,
        float scale,
        const Color& litTint,
        const Color& unlitTint,
        int sortKey) const;
    void SubmitDotText(
        const TickContext& context,
        std::string_view text,
        const Vector2& topLeft,
        float cellSize,
        const Color& tint,
        int sortKey) const;

    RuntimeServices* m_services = nullptr;
    FallingBlocksGame m_game;
    EntityId m_cameraEntityId = kInvalidEntityId;
    std::size_t m_actionSubscriptionId = 0;
    bool m_loggedControls = false;
    double m_gravityAccumulator = 0.0;
    int m_horizontalDirection = 0;
    double m_horizontalHoldSeconds = 0.0;
    bool m_horizontalRepeatArmed = false;
    double m_softDropHoldSeconds = 0.0;
    bool m_softDropRepeatArmed = false;
    std::string m_latestDebugReport;
    std::string m_latestAuthoringContext;
    std::string m_latestGameplayDigest;
    std::optional<RenderFrameSnapshot> m_lastCompletedFrame;
};
} // namespace she
