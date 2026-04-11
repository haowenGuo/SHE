#pragma once

#include "SHE/Core/MathTypes.hpp"

#include <string>

namespace she
{
// These components are bootstrap teaching types. They are intentionally small
// and readable so contributors can understand where scene data should live
// before a full ECS is integrated.
struct NameComponent
{
    std::string value;
};

struct TransformComponent
{
    Vector2 position{};
    float rotationDegrees = 0.0F;
    Vector2 scale{1.0F, 1.0F};
};

struct SpriteComponent
{
    std::string materialName;
};
} // namespace she

