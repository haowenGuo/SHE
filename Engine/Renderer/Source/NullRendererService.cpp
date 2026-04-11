#include "SHE/Renderer/NullRendererService.hpp"

#include "SHE/Core/Logger.hpp"

namespace she
{
void NullRendererService::Initialize(const ApplicationConfig&)
{
    m_lastFrameIndex = 0;
    m_lastSceneName.clear();
    m_lastEntityCount = 0;

    SHE_LOG_INFO("Renderer", "Null renderer initialized.");
}

void NullRendererService::Shutdown()
{
    SHE_LOG_INFO("Renderer", "Null renderer shutdown complete.");
}

void NullRendererService::BeginFrame(const FrameIndex frameIndex)
{
    m_lastFrameIndex = frameIndex;
}

void NullRendererService::SubmitSceneSnapshot(const std::string_view sceneName, const std::size_t entityCount)
{
    m_lastSceneName = std::string(sceneName);
    m_lastEntityCount = entityCount;
}

void NullRendererService::EndFrame()
{
}
} // namespace she

