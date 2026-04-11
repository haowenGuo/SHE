#pragma once

#include "SHE/Core/RuntimeServices.hpp"
#include "SHE/Scene/Components.hpp"
#include "SHE/Scene/Entity.hpp"

#include <string>
#include <vector>

namespace she
{
struct EntityRecord
{
    Entity entity;
    NameComponent name;
    TransformComponent transform;
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
    [[nodiscard]] std::size_t GetEntityCount() const override;
    void UpdateSceneGraph(double deltaSeconds) override;

    [[nodiscard]] const std::vector<EntityRecord>& GetEntities() const;

private:
    std::string m_activeSceneName = "BootstrapScene";
    EntityId m_nextEntityId = 1;
    std::vector<EntityRecord> m_entities;
    double m_lastSceneDelta = 0.0;
};
} // namespace she

