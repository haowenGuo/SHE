#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <string>
#include <unordered_map>

namespace she
{
struct AssetRecord
{
    AssetId id = kInvalidAssetId;
    std::string logicalName;
    std::string sourcePath;
};

// AssetManager is the bootstrap version of the asset registry.
// It does not load textures or sounds yet. Its job in Phase 1 is to teach the
// project that assets have stable logical identities and should not be fetched
// by arbitrary path strings all over the gameplay code.
class AssetManager final : public IAssetService
{
public:
    void Initialize() override;
    void Shutdown() override;
    AssetId RegisterAsset(std::string logicalName, std::string sourcePath) override;
    [[nodiscard]] bool HasAsset(std::string_view logicalName) const override;
    [[nodiscard]] std::size_t GetAssetCount() const override;

private:
    AssetId m_nextAssetId = 1;
    std::unordered_map<std::string, AssetRecord> m_assetsByName;
};
} // namespace she

