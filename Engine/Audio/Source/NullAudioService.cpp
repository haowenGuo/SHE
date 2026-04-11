#include "SHE/Audio/NullAudioService.hpp"

#include "SHE/Core/Logger.hpp"

namespace she
{
void NullAudioService::Initialize()
{
    m_lastDeltaSeconds = 0.0;
    SHE_LOG_INFO("Audio", "Null audio service initialized.");
}

void NullAudioService::Shutdown()
{
    SHE_LOG_INFO("Audio", "Null audio service shutdown complete.");
}

void NullAudioService::Update(const double deltaSeconds)
{
    m_lastDeltaSeconds = deltaSeconds;
}
} // namespace she

