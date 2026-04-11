#pragma once

#include <cstddef>
#include <cstdint>

namespace she
{
using AssetId = std::uint64_t;
using EntityId = std::uint32_t;
using FrameIndex = std::uint64_t;

constexpr AssetId kInvalidAssetId = 0;
constexpr EntityId kInvalidEntityId = 0;
} // namespace she

