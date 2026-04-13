#pragma once

#include "SHE/Renderer/Renderer2DService.hpp"

namespace she
{
// Keep the legacy header around so older bootstrap docs and worktrees can
// include it while the runtime moves to the first real 2D renderer path.
using NullRendererService = Renderer2DService;
} // namespace she

