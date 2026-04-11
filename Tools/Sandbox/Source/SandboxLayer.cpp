#include "SandboxLayer.hpp"

#include "SHE/Core/Logger.hpp"

namespace she
{
SandboxLayer::SandboxLayer() : Layer("SandboxLayer")
{
}

void SandboxLayer::OnAttach(RuntimeServices& services)
{
    services.scene->SetActiveScene("SandboxScene");
    services.scene->CreateEntity("SandboxCamera");
    services.scene->CreateEntity("DebugSprite");

    SHE_LOG_INFO("Sandbox", "Sandbox layer attached.");
}

void SandboxLayer::OnUpdate(const TickContext& context)
{
    if (!m_loggedSnapshot)
    {
        m_loggedSnapshot = true;
        SHE_LOG_INFO(
            "Sandbox",
            "Sandbox saw the first update frame. This executable is the right home for engine experiments.");
    }

    if (context.frameIndex == 1)
    {
        SHE_LOG_INFO("Sandbox", "Second frame reached in sandbox runtime.");
    }
}
} // namespace she

