#include "SandboxLayer.hpp"

#include "SHE/Core/Logger.hpp"

namespace she
{
SandboxLayer::SandboxLayer() : Layer("SandboxLayer")
{
}

void SandboxLayer::OnAttach(RuntimeServices& services)
{
    services.ai->SetAuthoringIntent("Use the sandbox to inspect engine modules, frame traces, and AI context exports.");
    services.scene->SetActiveScene("SandboxScene");
    services.scene->CreateEntity("SandboxCamera");
    services.scene->CreateEntity("DebugSprite");
    services.reflection->RegisterFeature(
        "SandboxFeature",
        "Tools/Sandbox",
        "Inspection layer for engine runtime and authoring context.");
    services.data->RegisterSchema(
        "sandbox.debug.panel",
        "Schema for future sandbox inspection panels.",
        {"id", "title", "widgets"});
    services.scripting->RegisterScriptModule(
        "sandbox.debug_tools",
        "Future Lua module for toggling debug panels and probes.");
    services.gameplay->QueueEvent("sandbox", "Attached", "Sandbox registered debug contracts.");

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
