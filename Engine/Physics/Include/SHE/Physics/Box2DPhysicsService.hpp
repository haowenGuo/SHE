#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <cstddef>
#include <memory>

namespace she
{
// Box2DPhysicsService owns the first real fixed-step simulation path. It binds
// Box2D bodies to W05 scene entities and forwards collision callbacks through
// the W01 gameplay event contract instead of inventing a parallel bus.
class Box2DPhysicsService final : public IPhysicsService
{
public:
    Box2DPhysicsService(std::shared_ptr<ISceneService> scene, std::shared_ptr<IGameplayService> gameplay);
    ~Box2DPhysicsService() override;

    void Initialize() override;
    void Shutdown() override;
    bool CreateBody(EntityId entityId, const PhysicsBodyDefinition& definition) override;
    bool DestroyBody(EntityId entityId) override;
    [[nodiscard]] bool HasBody(EntityId entityId) const override;
    [[nodiscard]] PhysicsColliderId CreateBoxCollider(EntityId entityId, const PhysicsBoxColliderDefinition& definition) override;
    bool DestroyCollider(PhysicsColliderId colliderId) override;
    void Step(double fixedTimeStep) override;
    [[nodiscard]] std::string BuildDebugSummary() const override;

    [[nodiscard]] std::size_t GetBodyCount() const;
    [[nodiscard]] std::size_t GetColliderCount() const;
    [[nodiscard]] std::size_t GetStepCount() const;

private:
    struct Impl;

    std::shared_ptr<ISceneService> m_scene;
    std::shared_ptr<IGameplayService> m_gameplay;
    std::unique_ptr<Impl> m_impl;
};
} // namespace she
