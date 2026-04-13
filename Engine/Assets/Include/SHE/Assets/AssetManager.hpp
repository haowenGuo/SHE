#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <map>

namespace she
{
// AssetManager keeps the stable identity and lookup rules for assets even
// before W08/W10 provide concrete renderer/audio resource objects.
class AssetManager final : public IAssetService
{
public:
    void Initialize() override;
    void Shutdown() override;
    AssetId RegisterAsset(std::string logicalName, std::string sourcePath) override;
    AssetId RegisterAsset(AssetMetadata metadata) override;
    void RegisterLoader(AssetLoaderDescriptor descriptor) override;
    [[nodiscard]] bool HasAsset(std::string_view logicalName) const override;
    [[nodiscard]] bool HasLoader(std::string_view loaderKey) const override;
    [[nodiscard]] AssetId FindAssetId(std::string_view logicalName) const override;
    [[nodiscard]] std::optional<AssetMetadata> FindAssetMetadata(AssetId assetId) const override;
    [[nodiscard]] std::optional<AssetLoaderDescriptor> ResolveLoader(AssetId assetId) const override;
    [[nodiscard]] AssetHandle AcquireAsset(AssetId assetId) override;
    void ReleaseAsset(AssetHandle handle) override;
    [[nodiscard]] std::size_t GetLiveHandleCount(AssetId assetId) const override;
    [[nodiscard]] std::vector<AssetRegistryEntry> ListAssets() const override;
    [[nodiscard]] std::vector<AssetLoaderDescriptor> ListLoaders() const override;
    [[nodiscard]] std::string DescribeRegistry() const override;
    [[nodiscard]] std::string DescribeLoaders() const override;
    [[nodiscard]] std::size_t GetAssetCount() const override;

private:
    AssetId m_nextAssetId = 1;
    std::size_t m_nextLeaseId = 1;
    std::map<std::string, AssetMetadata> m_assetsByName;
    std::map<AssetId, std::string> m_assetNamesById;
    std::map<std::string, AssetLoaderDescriptor> m_loadersByKey;
    std::map<std::size_t, AssetId> m_liveLeases;
    std::map<AssetId, std::size_t> m_liveHandleCounts;
};
} // namespace she

