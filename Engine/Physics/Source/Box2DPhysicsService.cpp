#include "SHE/Physics/Box2DPhysicsService.hpp"

#include "SHE/Core/Logger.hpp"

#include <box2d/box2d.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace she
{
namespace
{
constexpr float kDefaultGravityY = -9.8F;
constexpr int kVelocityIterations = 8;
constexpr int kPositionIterations = 3;
constexpr float kPositionTolerance = 0.0005F;
constexpr float kRotationToleranceDegrees = 0.05F;
constexpr float kPi = 3.14159265358979323846F;

struct PendingCollisionEvent
{
    PhysicsColliderId colliderIdA = kInvalidPhysicsColliderId;
    PhysicsColliderId colliderIdB = kInvalidPhysicsColliderId;
    const char* eventName = "";
};

struct ColliderRecord
{
    PhysicsColliderId colliderId = kInvalidPhysicsColliderId;
    EntityId entityId = kInvalidEntityId;
    PhysicsBoxColliderDefinition definition{};
    b2Fixture* fixture = nullptr;
};

struct BodyRecord
{
    EntityId entityId = kInvalidEntityId;
    PhysicsBodyDefinition definition{};
    b2Body* body = nullptr;
    std::vector<PhysicsColliderId> colliderIds;
    SceneTransform lastSceneTransform{};
};

[[nodiscard]] float DegreesToRadians(const float degrees)
{
    return degrees * (kPi / 180.0F);
}

[[nodiscard]] float RadiansToDegrees(const float radians)
{
    return radians * (180.0F / kPi);
}

[[nodiscard]] bool NearlyEqual(const float left, const float right, const float tolerance)
{
    return std::fabs(left - right) <= tolerance;
}

[[nodiscard]] bool NeedsTransformSync(const SceneTransform& transform, const b2Body& body)
{
    const b2Vec2 bodyPosition = body.GetPosition();
    const float bodyRotationDegrees = RadiansToDegrees(body.GetAngle());
    return !NearlyEqual(transform.position.x, bodyPosition.x, kPositionTolerance) ||
           !NearlyEqual(transform.position.y, bodyPosition.y, kPositionTolerance) ||
           !NearlyEqual(transform.rotationDegrees, bodyRotationDegrees, kRotationToleranceDegrees);
}

[[nodiscard]] b2BodyType ToBox2DBodyType(const PhysicsBodyType bodyType)
{
    switch (bodyType)
    {
    case PhysicsBodyType::Static:
        return b2_staticBody;
    case PhysicsBodyType::Kinematic:
        return b2_kinematicBody;
    case PhysicsBodyType::Dynamic:
        return b2_dynamicBody;
    default:
        return b2_staticBody;
    }
}

[[nodiscard]] std::string BoolToString(const bool value)
{
    return value ? "true" : "false";
}

[[nodiscard]] ColliderRecord* ResolveColliderRecord(b2Fixture* fixture)
{
    if (fixture == nullptr)
    {
        return nullptr;
    }

    const std::uintptr_t userData = fixture->GetUserData().pointer;
    return userData == 0 ? nullptr : reinterpret_cast<ColliderRecord*>(userData);
}

class GameplayBridgeContactListener final : public b2ContactListener
{
public:
    explicit GameplayBridgeContactListener(std::vector<PendingCollisionEvent>& pendingEvents)
        : m_pendingEvents(pendingEvents)
    {
    }

    void BeginContact(b2Contact* contact) override
    {
        QueueEvent(contact, "CollisionStarted");
    }

    void EndContact(b2Contact* contact) override
    {
        QueueEvent(contact, "CollisionEnded");
    }

private:
    void QueueEvent(b2Contact* contact, const char* eventName)
    {
        if (contact == nullptr)
        {
            return;
        }

        ColliderRecord* colliderA = ResolveColliderRecord(contact->GetFixtureA());
        ColliderRecord* colliderB = ResolveColliderRecord(contact->GetFixtureB());
        if (colliderA == nullptr || colliderB == nullptr)
        {
            return;
        }

        m_pendingEvents.push_back(PendingCollisionEvent{
            colliderA->colliderId,
            colliderB->colliderId,
            eventName,
        });
    }

    std::vector<PendingCollisionEvent>& m_pendingEvents;
};
} // namespace

struct Box2DPhysicsService::Impl
{
    Impl()
        : contactListener(pendingCollisionEvents)
    {
    }

    std::unique_ptr<b2World> world;
    std::unordered_map<EntityId, BodyRecord> bodies;
    std::unordered_map<PhysicsColliderId, ColliderRecord> colliders;
    std::vector<PendingCollisionEvent> pendingCollisionEvents;
    GameplayBridgeContactListener contactListener;
    PhysicsColliderId nextColliderId = 1;
    std::size_t stepCount = 0;
};

Box2DPhysicsService::Box2DPhysicsService(
    std::shared_ptr<ISceneService> scene,
    std::shared_ptr<IGameplayService> gameplay)
    : m_scene(std::move(scene))
    , m_gameplay(std::move(gameplay))
    , m_impl(std::make_unique<Impl>())
{
}

Box2DPhysicsService::~Box2DPhysicsService() = default;

void Box2DPhysicsService::Initialize()
{
    Shutdown();

    m_impl->world = std::make_unique<b2World>(b2Vec2{0.0F, kDefaultGravityY});
    m_impl->world->SetContactListener(&m_impl->contactListener);
    m_impl->nextColliderId = 1;
    m_impl->stepCount = 0;

    SHE_LOG_INFO("Physics", "Box2D physics service initialized.");
}

void Box2DPhysicsService::Shutdown()
{
    if (m_impl->world != nullptr)
    {
        m_impl->world->SetContactListener(nullptr);
    }

    m_impl->pendingCollisionEvents.clear();
    m_impl->colliders.clear();
    m_impl->bodies.clear();
    m_impl->world.reset();
    m_impl->nextColliderId = 1;
    m_impl->stepCount = 0;
}

bool Box2DPhysicsService::CreateBody(const EntityId entityId, const PhysicsBodyDefinition& definition)
{
    if (m_impl->world == nullptr || m_scene == nullptr || entityId == kInvalidEntityId || m_impl->bodies.contains(entityId))
    {
        return false;
    }

    SceneEntityView entityView;
    if (!m_scene->TryGetEntityView(entityId, entityView))
    {
        return false;
    }

    BodyRecord bodyRecord;
    bodyRecord.entityId = entityId;
    bodyRecord.definition = definition;
    bodyRecord.lastSceneTransform = entityView.transform;

    const auto [bodyIt, inserted] = m_impl->bodies.emplace(entityId, std::move(bodyRecord));
    if (!inserted)
    {
        return false;
    }

    b2BodyDef bodyDefinition{};
    bodyDefinition.type = ToBox2DBodyType(definition.bodyType);
    bodyDefinition.position.Set(entityView.transform.position.x, entityView.transform.position.y);
    bodyDefinition.angle = DegreesToRadians(entityView.transform.rotationDegrees);
    bodyDefinition.linearVelocity.Set(definition.linearVelocity.x, definition.linearVelocity.y);
    bodyDefinition.angularVelocity = DegreesToRadians(definition.angularVelocityDegrees);
    bodyDefinition.linearDamping = definition.linearDamping;
    bodyDefinition.angularDamping = definition.angularDamping;
    bodyDefinition.gravityScale = definition.gravityScale;
    bodyDefinition.fixedRotation = definition.fixedRotation;
    bodyDefinition.bullet = definition.isBullet;
    bodyDefinition.allowSleep = definition.allowSleep;
    bodyDefinition.userData.pointer = reinterpret_cast<std::uintptr_t>(&bodyIt->second);

    bodyIt->second.body = m_impl->world->CreateBody(&bodyDefinition);
    if (bodyIt->second.body == nullptr)
    {
        m_impl->bodies.erase(bodyIt);
        return false;
    }

    return true;
}

bool Box2DPhysicsService::DestroyBody(const EntityId entityId)
{
    const auto bodyIt = m_impl->bodies.find(entityId);
    if (bodyIt == m_impl->bodies.end())
    {
        return false;
    }

    if (bodyIt->second.body != nullptr && m_impl->world != nullptr)
    {
        m_impl->world->DestroyBody(bodyIt->second.body);
    }

    for (const PhysicsColliderId colliderId : bodyIt->second.colliderIds)
    {
        m_impl->colliders.erase(colliderId);
    }

    m_impl->bodies.erase(bodyIt);
    return true;
}

bool Box2DPhysicsService::HasBody(const EntityId entityId) const
{
    return m_impl->bodies.contains(entityId);
}

PhysicsColliderId Box2DPhysicsService::CreateBoxCollider(
    const EntityId entityId,
    const PhysicsBoxColliderDefinition& definition)
{
    if (m_impl->world == nullptr || definition.halfExtents.x <= 0.0F || definition.halfExtents.y <= 0.0F)
    {
        return kInvalidPhysicsColliderId;
    }

    auto bodyIt = m_impl->bodies.find(entityId);
    if (bodyIt == m_impl->bodies.end() || bodyIt->second.body == nullptr)
    {
        return kInvalidPhysicsColliderId;
    }

    const PhysicsColliderId colliderId = m_impl->nextColliderId++;
    ColliderRecord colliderRecord;
    colliderRecord.colliderId = colliderId;
    colliderRecord.entityId = entityId;
    colliderRecord.definition = definition;

    const auto [colliderIt, inserted] = m_impl->colliders.emplace(colliderId, std::move(colliderRecord));
    if (!inserted)
    {
        return kInvalidPhysicsColliderId;
    }

    b2PolygonShape polygon;
    polygon.SetAsBox(
        definition.halfExtents.x,
        definition.halfExtents.y,
        b2Vec2{definition.offset.x, definition.offset.y},
        DegreesToRadians(definition.rotationDegrees));

    b2FixtureDef fixtureDefinition{};
    fixtureDefinition.shape = &polygon;
    fixtureDefinition.density = definition.density;
    fixtureDefinition.friction = definition.friction;
    fixtureDefinition.restitution = definition.restitution;
    fixtureDefinition.isSensor = definition.isSensor;
    fixtureDefinition.userData.pointer = reinterpret_cast<std::uintptr_t>(&colliderIt->second);

    colliderIt->second.fixture = bodyIt->second.body->CreateFixture(&fixtureDefinition);
    if (colliderIt->second.fixture == nullptr)
    {
        m_impl->colliders.erase(colliderIt);
        return kInvalidPhysicsColliderId;
    }

    bodyIt->second.colliderIds.push_back(colliderId);
    return colliderId;
}

bool Box2DPhysicsService::DestroyCollider(const PhysicsColliderId colliderId)
{
    auto colliderIt = m_impl->colliders.find(colliderId);
    if (colliderIt == m_impl->colliders.end())
    {
        return false;
    }

    auto bodyIt = m_impl->bodies.find(colliderIt->second.entityId);
    if (bodyIt != m_impl->bodies.end() && bodyIt->second.body != nullptr && colliderIt->second.fixture != nullptr)
    {
        bodyIt->second.body->DestroyFixture(colliderIt->second.fixture);
        bodyIt->second.colliderIds.erase(
            std::remove(bodyIt->second.colliderIds.begin(), bodyIt->second.colliderIds.end(), colliderId),
            bodyIt->second.colliderIds.end());
    }

    m_impl->colliders.erase(colliderIt);
    return true;
}

void Box2DPhysicsService::Step(const double fixedTimeStep)
{
    if (m_impl->world == nullptr || m_scene == nullptr || fixedTimeStep <= 0.0)
    {
        return;
    }

    std::vector<EntityId> orphanedBodies;
    orphanedBodies.reserve(m_impl->bodies.size());

    for (auto& [entityId, bodyRecord] : m_impl->bodies)
    {
        if (bodyRecord.body == nullptr)
        {
            orphanedBodies.push_back(entityId);
            continue;
        }

        SceneEntityView entityView;
        if (!m_scene->TryGetEntityView(entityId, entityView))
        {
            orphanedBodies.push_back(entityId);
            continue;
        }

        bodyRecord.lastSceneTransform.scale = entityView.transform.scale;
        if (NeedsTransformSync(entityView.transform, *bodyRecord.body))
        {
            bodyRecord.body->SetTransform(
                b2Vec2{entityView.transform.position.x, entityView.transform.position.y},
                DegreesToRadians(entityView.transform.rotationDegrees));
        }
    }

    for (const EntityId entityId : orphanedBodies)
    {
        DestroyBody(entityId);
    }

    m_impl->pendingCollisionEvents.clear();
    m_impl->world->Step(static_cast<float>(fixedTimeStep), kVelocityIterations, kPositionIterations);

    for (auto& [entityId, bodyRecord] : m_impl->bodies)
    {
        if (bodyRecord.body == nullptr)
        {
            continue;
        }

        const b2Vec2 bodyPosition = bodyRecord.body->GetPosition();
        bodyRecord.lastSceneTransform.position = Vector2{bodyPosition.x, bodyPosition.y};
        bodyRecord.lastSceneTransform.rotationDegrees = RadiansToDegrees(bodyRecord.body->GetAngle());
        m_scene->TrySetEntityTransform(entityId, bodyRecord.lastSceneTransform);
    }

    if (m_gameplay != nullptr)
    {
        for (const PendingCollisionEvent& collisionEvent : m_impl->pendingCollisionEvents)
        {
            const auto colliderItA = m_impl->colliders.find(collisionEvent.colliderIdA);
            const auto colliderItB = m_impl->colliders.find(collisionEvent.colliderIdB);
            if (colliderItA == m_impl->colliders.end() || colliderItB == m_impl->colliders.end())
            {
                continue;
            }

            const ColliderRecord* colliderA = &colliderItA->second;
            const ColliderRecord* colliderB = &colliderItB->second;
            if (colliderB->entityId < colliderA->entityId)
            {
                std::swap(colliderA, colliderB);
            }

            SceneEntityView entityViewA;
            SceneEntityView entityViewB;
            const bool hasEntityViewA = m_scene->TryGetEntityView(colliderA->entityId, entityViewA);
            const bool hasEntityViewB = m_scene->TryGetEntityView(colliderB->entityId, entityViewB);

            std::ostringstream payload;
            payload << "entity_a=" << colliderA->entityId << "; entity_name_a="
                    << (hasEntityViewA ? entityViewA.entityName : "unknown") << "; collider_a=" << colliderA->colliderId
                    << "; sensor_a=" << BoolToString(colliderA->definition.isSensor) << "; entity_b=" << colliderB->entityId
                    << "; entity_name_b=" << (hasEntityViewB ? entityViewB.entityName : "unknown") << "; collider_b="
                    << colliderB->colliderId << "; sensor_b=" << BoolToString(colliderB->definition.isSensor);
            m_gameplay->QueueEvent("physics", collisionEvent.eventName, payload.str());
        }
    }

    m_impl->pendingCollisionEvents.clear();
    ++m_impl->stepCount;
}

std::size_t Box2DPhysicsService::GetBodyCount() const
{
    return m_impl->bodies.size();
}

std::size_t Box2DPhysicsService::GetColliderCount() const
{
    return m_impl->colliders.size();
}

std::size_t Box2DPhysicsService::GetStepCount() const
{
    return m_impl->stepCount;
}
} // namespace she
