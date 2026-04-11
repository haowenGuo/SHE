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

