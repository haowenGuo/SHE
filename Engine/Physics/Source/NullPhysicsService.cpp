#include "SHE/Physics/NullPhysicsService.hpp"

#include "SHE/Core/Logger.hpp"

namespace she
{
void NullPhysicsService::Initialize()
{
    m_stepCount = 0;
    m_lastFixedTimeStep = 0.0;
    SHE_LOG_INFO("Physics", "Null physics service initialized.");
}

void NullPhysicsService::Shutdown()
{
    SHE_LOG_INFO("Physics", "Null physics service shutdown complete.");
}

bool NullPhysicsService::CreateBody(const EntityId, const PhysicsBodyDefinition&)
{
    return false;
}

bool NullPhysicsService::DestroyBody(const EntityId)
{
    return false;
}

bool NullPhysicsService::HasBody(const EntityId) const
{
    return false;
}

PhysicsColliderId NullPhysicsService::CreateBoxCollider(const EntityId, const PhysicsBoxColliderDefinition&)
{
    return kInvalidPhysicsColliderId;
}

bool NullPhysicsService::DestroyCollider(const PhysicsColliderId)
{
    return false;
}

void NullPhysicsService::Step(const double fixedTimeStep)
{
    ++m_stepCount;
    m_lastFixedTimeStep = fixedTimeStep;
}

std::size_t NullPhysicsService::GetStepCount() const
{
    return m_stepCount;
}
} // namespace she

