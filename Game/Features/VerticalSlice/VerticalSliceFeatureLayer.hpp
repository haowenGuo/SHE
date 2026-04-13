#pragma once

#include "SHE/Core/Layer.hpp"
#include "SHE/Core/RuntimeServices.hpp"

#include <optional>
#include <string>
#include <vector>

namespace she
{
enum class VerticalSliceRoundState
{
    Booting,
    Playing,
    Won,
    Lost
};

[[nodiscard]] const char* GetVerticalSliceRoundStateName(VerticalSliceRoundState state);

struct VerticalSliceSnapshot
{
    VerticalSliceRoundState roundState = VerticalSliceRoundState::Booting;
    int collectedPickups = 0;
    int targetPickups = 0;
    std::size_t activeHazardCount = 0;
    std::size_t spawnedWaveCount = 0;
    double remainingSeconds = 0.0;
    bool roundStarted = false;
    bool scriptModuleLoaded = false;
    EntityId playerEntityId = kInvalidEntityId;
    EntityId cameraEntityId = kInvalidEntityId;
    EntityId activePickupEntityId = kInvalidEntityId;
    std::vector<EntityId> hazardEntityIds;
    std::string activePickupRecordId;
    std::string latestStatus;
};

class VerticalSliceFeatureLayer final : public Layer
{
public:
    VerticalSliceFeatureLayer();

    void OnAttach(RuntimeServices& services) override;
    void OnFixedUpdate(const TickContext& context) override;
    void OnUpdate(const TickContext& context) override;
    void OnRender(const TickContext& context) override;
    void OnUi(const TickContext& context) override;
    void OnDetach(RuntimeServices& services) override;

    [[nodiscard]] VerticalSliceSnapshot BuildSnapshot() const;
    [[nodiscard]] const std::string& GetLatestDebugReport() const;
    [[nodiscard]] const std::string& GetLatestAuthoringContext() const;
    [[nodiscard]] const std::string& GetLatestGameplayDigest() const;
    [[nodiscard]] const std::optional<RenderFrameSnapshot>& GetLastCompletedFrame() const;

private:
    struct HazardInstance
    {
        EntityId entityId = kInvalidEntityId;
        Vector2 velocity{};
        Vector2 size{1.0F, 1.0F};
        std::string recordId;
    };

    void RegisterFeatureContracts(RuntimeServices& services);
    void RegisterSliceData(RuntimeServices& services);
    void RegisterVisualAssets(RuntimeServices& services);
    void RegisterScriptContracts(RuntimeServices& services);
    void RegisterGameplayContracts(RuntimeServices& services);
    void CreatePersistentScene(RuntimeServices& services);
    void SubscribeToRuntimeEvents(RuntimeServices& services);
    void QueueRoundSequence(bool restart, std::string reason);
    bool QueueNextPickupScript();
    bool QueueNextWaveScript();
    void ResetRoundState(bool restart, const std::string& reason);
    void HandleSpawnPickupRequested(const GameplayEvent& event);
    void HandleSpawnWaveRequested(const GameplayEvent& event);
    void HandleTimerElapsed(const GameplayEvent& event);
    void HandleCollisionStarted(const GameplayEvent& event);
    void FinishRound(bool won, const std::string& reason);
    void UpdatePlayerMovement(double deltaSeconds);
    void UpdateHazards(double deltaSeconds);
    void UpdatePickupAnimation(double deltaSeconds);
    void UpdateHud();
    void SubmitEntitySprite(
        const TickContext& context,
        EntityId entityId,
        std::string_view materialName,
        const Color& tint,
        int sortKey) const;
    void DestroyActivePickup();
    void DestroyHazards();
    void QueueMusicStart();
    void QueueMusicStop();
    void QueueSfx(std::string_view assetName, std::string channelName);
    void CacheReports(RuntimeServices& services);

    RuntimeServices* m_services = nullptr;
    EntityId m_playerEntityId = kInvalidEntityId;
    EntityId m_cameraEntityId = kInvalidEntityId;
    EntityId m_arenaBackdropEntityId = kInvalidEntityId;
    EntityId m_timerTrackEntityId = kInvalidEntityId;
    EntityId m_timerFillEntityId = kInvalidEntityId;
    EntityId m_stateBeaconEntityId = kInvalidEntityId;
    EntityId m_scorePipEntityIds[3] = {kInvalidEntityId, kInvalidEntityId, kInvalidEntityId};
    EntityId m_activePickupEntityId = kInvalidEntityId;

    std::size_t m_startRoundSubscriptionId = 0;
    std::size_t m_restartRoundSubscriptionId = 0;
    std::size_t m_spawnPickupSubscriptionId = 0;
    std::size_t m_spawnWaveSubscriptionId = 0;
    std::size_t m_timerSubscriptionId = 0;
    std::size_t m_collisionSubscriptionId = 0;

    VerticalSliceRoundState m_roundState = VerticalSliceRoundState::Booting;
    int m_collectedPickups = 0;
    std::size_t m_spawnedWaveCount = 0;
    std::size_t m_nextPickupIndex = 0;
    std::size_t m_nextWaveIndex = 0;
    double m_remainingRoundSeconds = 0.0;
    double m_visualTimeSeconds = 0.0;
    bool m_roundStarted = false;
    bool m_loggedControls = false;
    bool m_scriptModuleLoaded = false;
    bool m_pendingPickupSpawn = false;
    bool m_pendingWaveSpawn = false;

    Vector2 m_activePickupBasePosition{};
    std::string m_activePickupRecordId;
    std::vector<HazardInstance> m_hazards;

    std::string m_latestStatus;
    std::string m_latestDebugReport;
    std::string m_latestAuthoringContext;
    std::string m_latestGameplayDigest;
    std::optional<RenderFrameSnapshot> m_lastCompletedFrame;
};
} // namespace she
