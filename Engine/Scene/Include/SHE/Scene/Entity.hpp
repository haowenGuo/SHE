#pragma once

#include "SHE/Core/Types.hpp"

namespace she
{
class Entity
{
public:
    Entity() = default;
    explicit Entity(EntityId id);

    [[nodiscard]] EntityId GetId() const;
    [[nodiscard]] explicit operator bool() const;

private:
    EntityId m_id = kInvalidEntityId;
};
} // namespace she

