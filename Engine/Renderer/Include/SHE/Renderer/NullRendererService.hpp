#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <string>

namespace she
{
// NullRendererService preserves the render-side contract without introducing a
// graphics backend yet. The important thing in Phase 1 is not pixels on screen;
// it is making the runtime flow and ownership boundaries easy to understand.
class NullRendererService final : public IRendererService
{
public:
    void Initialize(const ApplicationConfig& config) override;
    void Shutdown() override;
    void BeginFrame(FrameIndex frameIndex) override;
    void SubmitSceneSnapshot(std::string_view sceneName, std::size_t entityCount) override;
    void EndFrame() override;

private:
    FrameIndex m_lastFrameIndex = 0;
    std::string m_lastSceneName;
    std::size_t m_lastEntityCount = 0;
};
} // namespace she

