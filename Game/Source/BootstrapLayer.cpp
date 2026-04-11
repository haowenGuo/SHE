#include "BootstrapLayer.hpp"

#include "SHE/Core/Logger.hpp"

namespace she
{
BootstrapLayer::BootstrapLayer() : Layer("BootstrapLayer")
{
}

void BootstrapLayer::OnAttach(RuntimeServices& services)
{
    services.scene->SetActiveScene("BootstrapArena");

    services.assets->RegisterAsset("textures/player_idle", "Game/Assets/textures/player_idle.png");
    services.assets->RegisterAsset("audio/player_fire", "Game/Assets/audio/player_fire.wav");

    m_playerEntityId = services.scene->CreateEntity("Player");
    services.scene->CreateEntity("MainCamera");

    SHE_LOG_INFO("Game", "Bootstrap gameplay layer attached.");
}

void BootstrapLayer::OnUpdate(const TickContext& context)
{
    if (!m_loggedFrameSummary)
    {
        m_loggedFrameSummary = true;
        SHE_LOG_INFO(
            "Game",
            "First gameplay update reached. Scene, assets, and runtime services are now wired together.");
    }

    if (context.frameIndex == 0 && m_playerEntityId == kInvalidEntityId)
    {
        SHE_LOG_WARNING("Game", "Player entity was not created during bootstrap attach.");
    }
}

void BootstrapLayer::OnDetach(RuntimeServices&)
{
    SHE_LOG_INFO("Game", "Bootstrap gameplay layer detached.");
}
} // namespace she

