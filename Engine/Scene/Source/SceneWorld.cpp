#include "SHE/Scene/SceneWorld.hpp"

#include "SHE/Core/Logger.hpp"

#include <algorithm>
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
    ClearScene();

    SHE_LOG_INFO("Scene", "Scene world initialized.");
}

void SceneWorld::Shutdown()
{
    SHE_LOG_INFO("Scene", "Scene world shutdown complete.");
    ClearScene();
}

void SceneWorld::SetActiveScene(std::string sceneName)
{
    m_activeSceneName = std::move(sceneName);
    ClearScene();
}

std::string_view SceneWorld::GetActiveSceneName() const
{
    return m_activeSceneName;
}

EntityId SceneWorld::CreateEntity(std::string entityName)
{
    const EntityId id = m_nextEntityId++;
    m_entities.push_back(SceneEntitySlot{Entity{id}, false});
    m_nameComponents.emplace(id, NameComponent{std::move(entityName)});
    m_transformComponents.emplace(id, TransformComponent{});

    return id;
}

bool SceneWorld::DestroyEntity(const EntityId entityId)
{
    SceneEntitySlot* slot = FindEntitySlot(entityId);
    if (slot == nullptr || slot->pendingDestroy)
    {
        return false;
    }

    slot->pendingDestroy = true;
    m_pendingDestroy.push_back(entityId);
    return true;
}

bool SceneWorld::HasEntity(const EntityId entityId) const
{
    const SceneEntitySlot* slot = FindEntitySlot(entityId);
    return slot != nullptr && !slot->pendingDestroy;
}

bool SceneWorld::TryGetEntityView(const EntityId entityId, SceneEntityView& outEntityView) const
{
    if (!HasEntity(entityId))
    {
        return false;
    }

    const NameComponent* name = FindNameComponent(entityId);
    const TransformComponent* transform = FindTransformComponent(entityId);
    if (name == nullptr || transform == nullptr)
    {
        return false;
    }

    outEntityView = SceneEntityView{
        entityId,
        name->value,
        BuildSceneTransformView(*transform),
    };
    return true;
}

bool SceneWorld::TrySetEntityTransform(const EntityId entityId, const SceneTransform& transform)
{
    if (!HasEntity(entityId))
    {
        return false;
    }

    auto it = m_transformComponents.find(entityId);
    if (it == m_transformComponents.end())
    {
        return false;
    }

    it->second = BuildTransformComponent(transform);
    return true;
}

std::vector<SceneEntityView> SceneWorld::ListEntities() const
{
    std::vector<SceneEntityView> entities;
    entities.reserve(GetEntityCount());

    for (const SceneEntitySlot& slot : m_entities)
    {
        if (slot.pendingDestroy)
        {
            continue;
        }

        SceneEntityView entityView;
        if (TryGetEntityView(slot.entity.GetId(), entityView))
        {
            entities.push_back(std::move(entityView));
        }
    }

    return entities;
}

std::size_t SceneWorld::GetEntityCount() const
{
    return std::count_if(
        m_entities.begin(),
        m_entities.end(),
        [](const SceneEntitySlot& slot)
        {
            return !slot.pendingDestroy;
        });
}

void SceneWorld::UpdateSceneGraph(const double deltaSeconds)
{
    m_lastSceneDelta = deltaSeconds;
    PruneDestroyedEntities();
}

std::size_t SceneWorld::GetStoredEntityCount() const
{
    return m_entities.size();
}

std::size_t SceneWorld::GetPendingDestroyCount() const
{
    return m_pendingDestroy.size();
}

SceneEntitySlot* SceneWorld::FindEntitySlot(const EntityId entityId)
{
    const auto it = std::find_if(
        m_entities.begin(),
        m_entities.end(),
        [entityId](const SceneEntitySlot& slot)
        {
            return slot.entity.GetId() == entityId;
        });

    return it == m_entities.end() ? nullptr : &(*it);
}

const SceneEntitySlot* SceneWorld::FindEntitySlot(const EntityId entityId) const
{
    const auto it = std::find_if(
        m_entities.begin(),
        m_entities.end(),
        [entityId](const SceneEntitySlot& slot)
        {
            return slot.entity.GetId() == entityId;
        });

    return it == m_entities.end() ? nullptr : &(*it);
}

const NameComponent* SceneWorld::FindNameComponent(const EntityId entityId) const
{
    const auto it = m_nameComponents.find(entityId);
    return it == m_nameComponents.end() ? nullptr : &it->second;
}

const TransformComponent* SceneWorld::FindTransformComponent(const EntityId entityId) const
{
    const auto it = m_transformComponents.find(entityId);
    return it == m_transformComponents.end() ? nullptr : &it->second;
}

void SceneWorld::ClearScene()
{
    m_entities.clear();
    m_nameComponents.clear();
    m_transformComponents.clear();
    m_pendingDestroy.clear();
    m_lastSceneDelta = 0.0;
}

void SceneWorld::PruneDestroyedEntities()
{
    if (m_pendingDestroy.empty())
    {
        return;
    }

    for (const EntityId entityId : m_pendingDestroy)
    {
        m_nameComponents.erase(entityId);
        m_transformComponents.erase(entityId);
    }

    m_entities.erase(
        std::remove_if(
            m_entities.begin(),
            m_entities.end(),
            [](const SceneEntitySlot& slot)
            {
                return slot.pendingDestroy;
            }),
        m_entities.end());

    m_pendingDestroy.clear();
}

SceneTransform SceneWorld::BuildSceneTransformView(const TransformComponent& transform)
{
    return SceneTransform{
        transform.position,
        transform.rotationDegrees,
        transform.scale,
    };
}

TransformComponent SceneWorld::BuildTransformComponent(const SceneTransform& transform)
{
    return TransformComponent{
        transform.position,
        transform.rotationDegrees,
        transform.scale,
    };
}
} // namespace she
