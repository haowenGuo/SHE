#include "SHE/Gameplay/GameplayService.hpp"
#include "SHE/Physics/Box2DPhysicsService.hpp"
#include "SHE/Scene/SceneWorld.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace
{
bool Expect(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
    }

    return condition;
}

bool NearlyEqual(const float left, const float right)
{
    constexpr float kTolerance = 0.05F;
    return left - right < kTolerance && left - right > -kTolerance;
}

bool Contains(const std::string& value, const std::string& needle)
{
    return value.find(needle) != std::string::npos;
}

bool TestBodyAndColliderLifecycleTracksSceneLifetime()
{
    auto scene = std::make_shared<she::SceneWorld>();
    auto gameplay = std::make_shared<she::GameplayService>();
    she::Box2DPhysicsService physics(scene, gameplay);

    scene->Initialize();
    scene->SetActiveScene("PhysicsLifecycle");
    gameplay->Initialize();
    physics.Initialize();

    const she::EntityId groundId = scene->CreateEntity("Ground");
    const she::EntityId crateId = scene->CreateEntity("Crate");
    const bool wroteCrateTransform = scene->TrySetEntityTransform(
        crateId,
        she::SceneTransform{
            she::Vector2{0.0F, 3.0F},
            0.0F,
            she::Vector2{1.0F, 1.0F},
        });

    bool success = true;
    success = Expect(wroteCrateTransform, "Expected the crate entity transform to be writable before body creation.") && success;
    success = Expect(
                  physics.CreateBody(groundId, she::PhysicsBodyDefinition{she::PhysicsBodyType::Static}),
                  "Expected the ground entity to accept a static physics body.") &&
              success;
    success = Expect(
                  physics.CreateBody(
                      crateId,
                      she::PhysicsBodyDefinition{
                          she::PhysicsBodyType::Dynamic,
                          she::Vector2{},
                          0.0F,
                          0.0F,
                          0.0F,
                          1.0F,
                          false,
                          false,
                          true,
                      }),
                  "Expected the crate entity to accept a dynamic physics body.") &&
              success;

    const she::PhysicsColliderId groundColliderId = physics.CreateBoxCollider(
        groundId,
        she::PhysicsBoxColliderDefinition{
            she::Vector2{5.0F, 0.5F},
            she::Vector2{0.0F, 0.0F},
            0.0F,
            0.0F,
            0.2F,
            0.0F,
            false,
        });
    const she::PhysicsColliderId crateColliderId = physics.CreateBoxCollider(
        crateId,
        she::PhysicsBoxColliderDefinition{
            she::Vector2{0.5F, 0.5F},
            she::Vector2{0.0F, 0.0F},
            0.0F,
            1.0F,
            0.2F,
            0.0F,
            false,
        });

    success = Expect(groundColliderId != she::kInvalidPhysicsColliderId, "Expected the ground collider to be created.") && success;
    success = Expect(crateColliderId != she::kInvalidPhysicsColliderId, "Expected the crate collider to be created.") && success;
    success = Expect(physics.GetBodyCount() == 2, "Expected exactly two registered Box2D bodies.") && success;
    success = Expect(physics.GetColliderCount() == 2, "Expected exactly two registered Box2D colliders.") && success;
    success = Expect(physics.DestroyCollider(crateColliderId), "Expected the crate collider to be removable without destroying the body.") &&
              success;
    success = Expect(physics.GetColliderCount() == 1, "Expected collider destruction to update the collider registry.") && success;

    const she::PhysicsColliderId replacementColliderId = physics.CreateBoxCollider(
        crateId,
        she::PhysicsBoxColliderDefinition{
            she::Vector2{0.45F, 0.45F},
            she::Vector2{0.0F, 0.0F},
            0.0F,
            1.0F,
            0.2F,
            0.0F,
            false,
        });
    success = Expect(
                  replacementColliderId != she::kInvalidPhysicsColliderId && replacementColliderId != crateColliderId,
                  "Expected collider recreation to issue a fresh collider id.") &&
              success;
    success = Expect(physics.GetColliderCount() == 2, "Expected collider recreation to restore the collider count.") && success;

    scene->DestroyEntity(crateId);
    gameplay->BeginFrame(0);
    physics.Step(1.0 / 60.0);
    gameplay->FlushCommands();

    success = Expect(!physics.HasBody(crateId), "Expected destroyed scene entities to be pruned from the physics registry.") && success;
    success = Expect(physics.GetBodyCount() == 1, "Expected only the surviving ground body to remain registered.") && success;
    success = Expect(physics.GetColliderCount() == 1, "Expected collider cleanup to follow body cleanup.") && success;
    success = Expect(physics.DestroyBody(groundId), "Expected the surviving ground body to remain explicitly destroyable.") && success;
    success = Expect(physics.GetBodyCount() == 0, "Expected body destruction to fully clear the physics registry.") && success;
    success = Expect(physics.GetColliderCount() == 0, "Expected body destruction to remove all attached colliders.") && success;

    physics.Shutdown();
    gameplay->Shutdown();
    scene->Shutdown();
    return success;
}

bool TestFixedStepSynchronizesSceneAndRoutesCollisionEvents()
{
    auto scene = std::make_shared<she::SceneWorld>();
    auto gameplay = std::make_shared<she::GameplayService>();
    she::Box2DPhysicsService physics(scene, gameplay);

    scene->Initialize();
    scene->SetActiveScene("PhysicsCollision");
    gameplay->Initialize();
    physics.Initialize();

    const she::EntityId floorId = scene->CreateEntity("Floor");
    const she::EntityId playerId = scene->CreateEntity("Player");
    const bool wrotePlayerTransform = scene->TrySetEntityTransform(
        playerId,
        she::SceneTransform{
            she::Vector2{0.0F, 3.0F},
            0.0F,
            she::Vector2{1.0F, 1.0F},
        });

    bool success = true;
    success = Expect(wrotePlayerTransform, "Expected the player entity transform to be writable before body creation.") && success;
    success = Expect(
                  physics.CreateBody(floorId, she::PhysicsBodyDefinition{she::PhysicsBodyType::Static}),
                  "Expected the floor entity to accept a static body.") &&
              success;
    success = Expect(
                  physics.CreateBody(
                      playerId,
                      she::PhysicsBodyDefinition{
                          she::PhysicsBodyType::Dynamic,
                          she::Vector2{},
                          0.0F,
                          0.0F,
                          0.0F,
                          1.0F,
                          false,
                          false,
                          true,
                      }),
                  "Expected the player entity to accept a dynamic body.") &&
              success;

    const she::PhysicsColliderId floorColliderId = physics.CreateBoxCollider(
        floorId,
        she::PhysicsBoxColliderDefinition{
            she::Vector2{6.0F, 0.5F},
            she::Vector2{0.0F, 0.0F},
            0.0F,
            0.0F,
            0.4F,
            0.0F,
            false,
        });
    const she::PhysicsColliderId playerColliderId = physics.CreateBoxCollider(
        playerId,
        she::PhysicsBoxColliderDefinition{
            she::Vector2{0.5F, 0.5F},
            she::Vector2{0.0F, 0.0F},
            0.0F,
            1.0F,
            0.2F,
            0.0F,
            false,
        });

    success = Expect(floorColliderId != she::kInvalidPhysicsColliderId, "Expected the floor collider to be created.") && success;
    success = Expect(playerColliderId != she::kInvalidPhysicsColliderId, "Expected the player collider to be created.") && success;

    std::size_t collisionStartedCount = 0;
    std::size_t collisionEndedCount = 0;
    std::string lastStartedPayload;
    std::string lastEndedPayload;

    const std::size_t startedSubscriptionId = gameplay->SubscribeToEvent(
        "physics",
        "CollisionStarted",
        [&collisionStartedCount, &lastStartedPayload](const she::GameplayEvent& event)
        {
            ++collisionStartedCount;
            lastStartedPayload = event.payload;
        });
    const std::size_t endedSubscriptionId = gameplay->SubscribeToEvent(
        "physics",
        "CollisionEnded",
        [&collisionEndedCount, &lastEndedPayload](const she::GameplayEvent& event)
        {
            ++collisionEndedCount;
            lastEndedPayload = event.payload;
        });

    she::SceneEntityView playerView;
    for (she::FrameIndex frameIndex = 0; frameIndex < 120 && collisionStartedCount == 0; ++frameIndex)
    {
        gameplay->BeginFrame(frameIndex);
        physics.Step(1.0 / 60.0);
        gameplay->FlushCommands();
    }

    success = Expect(collisionStartedCount >= 1, "Expected the falling body to produce a collision start event.") && success;
    success = Expect(
                  Contains(lastStartedPayload, std::string("entity_a=") + std::to_string(floorId)) &&
                      Contains(lastStartedPayload, std::string("entity_b=") + std::to_string(playerId)),
                  "Expected collision start payloads to describe both participating entities.") &&
              success;
    success = Expect(physics.GetStepCount() >= 1, "Expected the physics service to record fixed-step advancement.") && success;
    success = Expect(scene->TryGetEntityView(playerId, playerView), "Expected the player entity view to remain queryable.") && success;
    success = Expect(
                  playerView.transform.position.y < 2.0F && playerView.transform.position.y > 0.8F,
                  "Expected Box2D simulation to write the falling body position back into the scene transform.") &&
              success;

    const bool teleportedPlayer = scene->TrySetEntityTransform(
        playerId,
        she::SceneTransform{
            she::Vector2{0.0F, 5.0F},
            0.0F,
            playerView.transform.scale,
        });
    success = Expect(teleportedPlayer, "Expected the player entity transform to accept a gameplay-authored teleport.") && success;

    gameplay->BeginFrame(200);
    physics.Step(1.0 / 60.0);
    gameplay->FlushCommands();

    success = Expect(collisionEndedCount >= 1, "Expected teleported separation to produce a collision end event.") && success;
    success = Expect(
                  Contains(lastEndedPayload, std::string("entity_a=") + std::to_string(floorId)) &&
                      Contains(lastEndedPayload, std::string("entity_b=") + std::to_string(playerId)),
                  "Expected collision end payloads to describe both participating entities.") &&
              success;
    success = Expect(scene->TryGetEntityView(playerId, playerView), "Expected the teleported player entity to remain queryable.") && success;
    success = Expect(
                  NearlyEqual(playerView.transform.position.x, 0.0F) && playerView.transform.position.y > 4.5F,
                  "Expected scene-authored teleports to push dynamic bodies back into Box2D before the next fixed step.") &&
              success;

    gameplay->UnsubscribeFromEvent(endedSubscriptionId);
    gameplay->UnsubscribeFromEvent(startedSubscriptionId);
    physics.Shutdown();
    gameplay->Shutdown();
    scene->Shutdown();
    return success;
}
} // namespace

int main()
{
    bool success = true;
    success = TestBodyAndColliderLifecycleTracksSceneLifetime() && success;
    success = TestFixedStepSynchronizesSceneAndRoutesCollisionEvents() && success;
    return success ? 0 : 1;
}
