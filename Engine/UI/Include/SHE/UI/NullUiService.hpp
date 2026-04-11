#pragma once

#include "SHE/Core/RuntimeServices.hpp"

namespace she
{
// NullUiService is where debug tooling will eventually plug into the runtime.
// Keeping it present now prevents UI work from becoming an afterthought later.
class NullUiService final : public IUiService
{
public:
    void Initialize() override;
    void Shutdown() override;
    void BeginFrame(FrameIndex frameIndex) override;
    void EndFrame() override;

private:
    FrameIndex m_lastFrameIndex = 0;
};
} // namespace she

