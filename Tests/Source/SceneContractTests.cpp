#include "SHE/Scene/SceneWorld.hpp"

#include <iostream>
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
    constexpr float kTolerance = 0.0001F;
    const float difference = left - right;
    return difference < kTolerance && difference > -kTolerance;
}

bool TestEntityLifetimeAndQueryBoundary()
{
    she::SceneWorld world;
    world.Initialize();
    world.SetActiveScene("LifetimeScene");

    const she::EntityId playerId = world.CreateEntity("Player");
    const she::EntityId cameraId = world.CreateEntity("MainCamera");

    she::SceneEntityView playerView;

    bool success = true;
    success = Expect(playerId != she::kInvalidEntityId, "Player entity should receive a valid id.") && success;
    success = Expect(cameraId != she::kInvalidEntityId, "Camera entity should receive a valid id.") && success;
    success = Expect(playerId != cameraId, "Entity ids should remain unique within a scene.") && success;
    success = Expect(world.GetEntityCount() == 2, "Two live entities should be visible after creation.") && success;
    success = Expect(world.GetStoredEntityCount() == 2, "Backing storage should contain both created entities.") && success;
    success = Expect(world.HasEntity(playerId), "Created player entity should be queryable.") && success;
    success = Expect(world.TryGetEntityView(playerId, playerView), "Player entity view should be readable.") && success;
    success = Expect(playerView.entityName == "Player", "Entity view should preserve the authored name.") && success;
    success = Expect(
                  NearlyEqual(playerView.transform.scale.x, 1.0F) &&
                      NearlyEqual(playerView.transform.scale.y, 1.0F),
                  "New entities should start with a unit transform scale.") &&
              success;

    const bool wroteTransform = world.TrySetEntityTransform(
        playerId,
        she::SceneTransform{
            she::Vector2{4.0F, 8.0F},
            15.0F,
            she::Vector2{2.0F, 3.0F},
        });
    success = Expect(wroteTransform, "Scene world should allow transform updates for live entities.") && success;
    success = Expect(world.TryGetEntityView(playerId, playerView), "Updated player view should remain readable.") && success;
    success = Expect(
                  NearlyEqual(playerView.transform.position.x, 4.0F) &&
                      NearlyEqual(playerView.transform.position.y, 8.0F) &&
                      NearlyEqual(playerView.transform.rotationDegrees, 15.0F) &&
                      NearlyEqual(playerView.transform.scale.x, 2.0F) &&
                      NearlyEqual(playerView.transform.scale.y, 3.0F),
                  "Transform queries should reflect the latest scene-owned values.") &&
              success;

    const auto listedEntities = world.ListEntities();
    success = Expect(listedEntities.size() == 2, "ListEntities should return every live entity in the active scene.") && success;
    if (listedEntities.size() == 2)
    {
        success = Expect(
                      listedEntities[0].entityId == playerId && listedEntities[1].entityId == cameraId,
                      "ListEntities should preserve creation order for stable downstream iteration.") &&
                  success;
    }

    const bool scheduledDestroy = world.DestroyEntity(playerId);
    success = Expect(scheduledDestroy, "DestroyEntity should accept live entities exactly once.") && success;
    success = Expect(!world.DestroyEntity(playerId), "DestroyEntity should reject already-destroyed entities.") && success;
    success = Expect(!world.HasEntity(playerId), "Destroyed entities should stop being externally visible immediately.") && success;
    success = Expect(
                  world.GetEntityCount() == 1,
                  "Entity count should reflect only live entities even before deferred cleanup runs.") &&
              success;
    success = Expect(
                  world.GetStoredEntityCount() == 2 && world.GetPendingDestroyCount() == 1,
                  "Deferred cleanup should keep pending destroys in storage until the scene update boundary.") &&
              success;
    success = Expect(
                  !world.TryGetEntityView(playerId, playerView),
                  "Destroyed entities should no longer produce component views.") &&
              success;
    success = Expect(
                  !world.TrySetEntityTransform(
                      playerId,
                      she::SceneTransform{she::Vector2{1.0F, 1.0F}, 0.0F, she::Vector2{1.0F, 1.0F}}),
                  "Destroyed entities should reject further transform writes.") &&
              success;

    world.UpdateSceneGraph(1.0 / 60.0);

    const auto survivingEntities = world.ListEntities();
    success = Expect(world.GetStoredEntityCount() == 1, "Scene update should prune destroyed entities from storage.") && success;
    success = Expect(world.GetPendingDestroyCount() == 0, "Scene update should drain the pending destroy queue.") && success;
    if (survivingEntities.size() == 1)
    {
        success = Expect(
                      survivingEntities[0].entityId == cameraId,
                      "Scene update should retain only the surviving live entities.") &&
                  success;
    }
    else
    {
        success = Expect(false, "Scene update should retain only one surviving live entity.") && success;
    }

    world.Shutdown();
    return success;
}

bool TestSceneSwitchClearsWorldButPreservesIdentityMonotonicity()
{
    she::SceneWorld world;
    world.Initialize();
    world.SetActiveScene("SceneA");

    const she::EntityId firstId = world.CreateEntity("First");
    const she::EntityId secondId = world.CreateEntity("Second");

    world.SetActiveScene("SceneB");

    bool success = true;
    success = Expect(world.GetActiveSceneName() == "SceneB", "Scene switch should update the active scene name.") && success;
    success = Expect(world.GetEntityCount() == 0, "Scene switch should clear live entities from the previous scene.") && success;
    success = Expect(world.GetStoredEntityCount() == 0, "Scene switch should clear backing storage.") && success;
    success = Expect(world.GetPendingDestroyCount() == 0, "Scene switch should drop pending scene mutations.") && success;

    const she::EntityId thirdId = world.CreateEntity("Third");
    success = Expect(
                  thirdId > secondId && thirdId > firstId,
                  "Entity ids should remain monotonic across scene switches so stale ids are not reused.") &&
              success;

    she::SceneEntityView thirdView;
    success = Expect(world.TryGetEntityView(thirdId, thirdView), "Entities in the new scene should remain queryable.") && success;
    success = Expect(thirdView.entityName == "Third", "Scene switch should not corrupt new entity component data.") && success;

    world.Shutdown();
    return success;
}
} // namespace

int main()
{
    bool success = true;
    success = TestEntityLifetimeAndQueryBoundary() && success;
    success = TestSceneSwitchClearsWorldButPreservesIdentityMonotonicity() && success;
    return success ? 0 : 1;
}
