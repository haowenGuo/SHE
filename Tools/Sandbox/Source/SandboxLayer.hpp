#pragma once

#include "SHE/Core/Layer.hpp"

namespace she
{
// SandboxLayer is the first non-game executable layer. It is the seed for a
// future sandbox/editor style tool where engine systems can be inspected
// without coupling that tooling to the shipping game executable.
class SandboxLayer final : public Layer
{
public:
    SandboxLayer();

    void OnAttach(RuntimeServices& services) override;
    void OnUpdate(const TickContext& context) override;

private:
    bool m_loggedSnapshot = false;
};
} // namespace she

