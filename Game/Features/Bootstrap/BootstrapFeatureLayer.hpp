#pragma once

#include "SHE/Core/Layer.hpp"
#include "SHE/Core/Types.hpp"

namespace she
{
// BootstrapFeatureLayer demonstrates the intended shape of a game feature in
// the AI-native architecture. It registers metadata, schemas, script modules,
// and gameplay primitives from one narrow feature boundary.
class BootstrapFeatureLayer final : public Layer
{
public:
    BootstrapFeatureLayer();

    void OnAttach(RuntimeServices& services) override;
    void OnFixedUpdate(const TickContext& context) override;
    void OnUpdate(const TickContext& context) override;
    void OnRender(const TickContext& context) override;
    void OnDetach(RuntimeServices& services) override;

private:
    EntityId m_playerEntityId = kInvalidEntityId;
    EntityId m_cameraEntityId = kInvalidEntityId;
    std::size_t m_timerSubscriptionId = 0;
    std::size_t m_observedSpawnPulseCount = 0;
    std::size_t m_handledSpawnPulseCount = 0;
    bool m_loggedFeatureSummary = false;
    bool m_queuedFirstSpawnCommand = false;
};
} // namespace she

