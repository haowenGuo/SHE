#pragma once

#include "SHE/Core/RuntimeServices.hpp"
#include "SHE/Core/Types.hpp"

namespace she
{
// TickContext is the data packet that flows through gameplay layers every frame.
// Keeping it small is intentional: layers should know the current timing data
// and how to reach engine services, but they should not own the services.
struct TickContext
{
    double deltaSeconds = 0.0;
    FrameIndex frameIndex = 0;
    bool isFixedStep = false;
    RuntimeServices& services;
};
} // namespace she

