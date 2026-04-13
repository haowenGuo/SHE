#pragma once

#include <cstddef>
#include <string>

namespace she
{
// ApplicationConfig owns the knobs that describe how the runtime should start.
// It is intentionally small and readable so contributors can see which decisions
// belong to startup policy rather than any one engine subsystem.
struct ApplicationConfig
{
    std::string applicationName = "SHE";
    int windowWidth = 1280;
    int windowHeight = 720;
    bool startWindowHidden = false;

    // The bootstrap repository runs with deterministic timing by default so
    // tests and early engine experiments are stable and easy to reason about.
    double targetFrameRate = 60.0;
    double fixedTimeStep = 1.0 / 60.0;
    double maxFrameDelta = 0.25;
    bool useDeterministicFrameTiming = true;

    // When maxFrames is greater than zero, the application exits automatically
    // after that many frames. This makes the bootstrap executables testable
    // before a real OS window backend exists.
    std::size_t maxFrames = 5;
};
} // namespace she

