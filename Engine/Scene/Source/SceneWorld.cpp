#include "SHE/Scene/SceneWorld.hpp"

#include "SHE/Core/Logger.hpp"

#include <utility>

namespace she
{
Entity::Entity(const EntityId id) : m_id(id)
{
}

EntityId Entity::GetId() const
{
    return m_id;
}

Entity::operator bool() const
{
    return m_id != kInvalidEntityId;
}

void SceneWorld::Initialize()
{
    m_activeSceneName = "BootstrapScene";
    m_nextEntityId = 1;
    m_entities.clear();
    m_lastSceneDelta = 0.0;

    SHE_LOG_INFO("Scene", "Scene world initialized.");
}

void SceneWorld::Shutdown()
{
    SHE_LOG_INFO("Scene", "Scene world shutdown complete.");
    m_entities.clear();
}

void SceneWorld::SetActiveScene(std::string sceneName)
{
    m_activeSceneName = std::move(sceneName);
}

std::string_view SceneWorld::GetActiveSceneName() const
{
    return m_activeSceneName;
}

EntityId SceneWorld::CreateEntity(std::string entityName)
{
    const EntityId id = m_nextEntityId++;
    m_entities.push_back(EntityRecord{
        Entity{id},
        NameComponent{std::move(entityName)},
        TransformComponent{},
    });

    return id;
}

std::size_t SceneWorld::GetEntityCount() const
{
    return m_entities.size();
}

void SceneWorld::UpdateSceneGraph(const double deltaSeconds)
{
    m_lastSceneDelta = deltaSeconds;
}

const std::vector<EntityRecord>& SceneWorld::GetEntities() const
{
    return m_entities;
}
} // namespace she
