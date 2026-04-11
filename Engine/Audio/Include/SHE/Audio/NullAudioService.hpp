#pragma once

#include "SHE/Core/RuntimeServices.hpp"

namespace she
{
// NullAudioService is the placeholder owner of audio frame updates. Keeping the
// contract separate from gameplay now makes it much easier to add buses,
// one-shots, and music later without mixing those concerns into Game/.
class NullAudioService final : public IAudioService
{
public:
    void Initialize() override;
    void Shutdown() override;
    void Update(double deltaSeconds) override;

private:
    double m_lastDeltaSeconds = 0.0;
};
} // namespace she

