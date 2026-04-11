#pragma once

#include "SHE/Core/Layer.hpp"
#include "SHE/Core/Types.hpp"

namespace she
{
// BootstrapLayer is the first gameplay-side example for the repository.
// Its purpose is less about fun gameplay and more about showing contributors how
// Game/ should talk to the engine: through layers and runtime services.
class BootstrapLayer final : public Layer
{
public:
    BootstrapLayer();

    void OnAttach(RuntimeServices& services) override;
    void OnUpdate(const TickContext& context) override;
    void OnDetach(RuntimeServices& services) override;

private:
    EntityId m_playerEntityId = kInvalidEntityId;
    bool m_loggedFrameSummary = false;
};
} // namespace she

