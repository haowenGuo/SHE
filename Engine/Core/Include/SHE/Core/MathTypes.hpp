#pragma once

namespace she
{
// Phase 1 keeps math deliberately tiny so the architecture stays readable.
// Later milestones will likely replace or extend these types with glm.
struct Vector2
{
    float x = 0.0F;
    float y = 0.0F;
};

struct Color
{
    float r = 1.0F;
    float g = 1.0F;
    float b = 1.0F;
    float a = 1.0F;
};
} // namespace she

