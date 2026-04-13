#pragma once

#include "SHE/Core/RuntimeServices.hpp"
#include "SHE/Scene/Components.hpp"
#include "SHE/Scene/Entity.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace she
{
struct SceneEntitySlot
{
    Entity entity;
    bool pendingDestroy = false;
};

// SceneWorld is the temporary scene store for Phase 1.
// It deliberately focuses on ownership and API shape rather than on final ECS
// performance. In a later phase we can swap its internals to EnTT while keeping
// the higher-level engine contracts recognizable.
class SceneWorld final : public ISceneService
{
public:
    void Initialize() override;
    void Shutdown() override;
    void SetActiveScene(std::string sceneName) override;
    [[nodiscard]] std::string_view GetActiveSceneName() const override;
    EntityId CreateEntity(std::string entityName) override;
    bool DestroyEntity(EntityId entityId) override;
    [[nodiscard]] bool HasEntity(EntityId entityId) const override;
    [[nodiscard]] bool TryGetEntityView(EntityId entityId, SceneEntityView& outEntityView) const override;
    [[nodiscard]] bool TrySetEntityTransform(EntityId entityId, const SceneTransform& transform) override;
    [[nodiscard]] std::vector<SceneEntityView> ListEntities() const override;
    [[nodiscard]] std::size_t GetEntityCount() const override;
    void UpdateSceneGraph(double deltaSeconds) override;

    [[nodiscard]] std::size_t GetStoredEntityCount() const;
    [[nodiscard]] std::size_t GetPendingDestroyCount() const;

private:
    [[nodiscard]] SceneEntitySlot* FindEntitySlot(EntityId entityId);
    [[nodiscard]] const SceneEntitySlot* FindEntitySlot(EntityId entityId) const;
    [[nodiscard]] const NameComponent* FindNameComponent(EntityId entityId) const;
    [[nodiscard]] const TransformComponent* FindTransformComponent(EntityId entityId) const;
    void ClearScene();
    void PruneDestroyedEntities();
    [[nodiscard]] static SceneTransform BuildSceneTransformView(const TransformComponent& transform);
    [[nodiscard]] static TransformComponent BuildTransformComponent(const SceneTransform& transform);

    std::string m_activeSceneName = "BootstrapScene";
    EntityId m_nextEntityId = 1;
    std::vector<SceneEntitySlot> m_entities;
    std::unordered_map<EntityId, NameComponent> m_nameComponents;
    std::unordered_map<EntityId, TransformComponent> m_transformComponents;
    std::vector<EntityId> m_pendingDestroy;
    double m_lastSceneDelta = 0.0;
};
} // namespace she

