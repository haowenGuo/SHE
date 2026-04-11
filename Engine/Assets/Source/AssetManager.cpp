#include "SHE/Assets/AssetManager.hpp"

#include "SHE/Core/Logger.hpp"

namespace she
{
void AssetManager::Initialize()
{
    m_nextAssetId = 1;
    m_assetsByName.clear();
    SHE_LOG_INFO("Assets", "Asset registry initialized.");
}

void AssetManager::Shutdown()
{
    SHE_LOG_INFO("Assets", "Asset registry shutdown complete.");
    m_assetsByName.clear();
}

AssetId AssetManager::RegisterAsset(std::string logicalName, std::string sourcePath)
{
    const auto [it, inserted] = m_assetsByName.emplace(
        logicalName,
        AssetRecord{m_nextAssetId, logicalName, sourcePath});

    if (inserted)
    {
        ++m_nextAssetId;
    }

    return it->second.id;
}

bool AssetManager::HasAsset(const std::string_view logicalName) const
{
    return m_assetsByName.contains(std::string(logicalName));
}

std::size_t AssetManager::GetAssetCount() const
{
    return m_assetsByName.size();
}
} // namespace she

