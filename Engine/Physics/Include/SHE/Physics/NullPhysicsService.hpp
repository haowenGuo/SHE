#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <cstddef>

namespace she
{
// NullPhysicsService exists so the fixed-step path has a real owner from day
// one. Later we can replace its internals with Box2D without rewriting the
// application loop or gameplay layers.
class NullPhysicsService final : public IPhysicsService
{
public:
    void Initialize() override;
    void Shutdown() override;
    bool CreateBody(EntityId entityId, const PhysicsBodyDefinition& definition) override;
    bool DestroyBody(EntityId entityId) override;
    [[nodiscard]] bool HasBody(EntityId entityId) const override;
    [[nodiscard]] PhysicsColliderId CreateBoxCollider(EntityId entityId, const PhysicsBoxColliderDefinition& definition) override;
    bool DestroyCollider(PhysicsColliderId colliderId) override;
    void Step(double fixedTimeStep) override;

    [[nodiscard]] std::size_t GetStepCount() const;

private:
    std::size_t m_stepCount = 0;
    double m_lastFixedTimeStep = 0.0;
};
} // namespace she

