#include "SHE/Assets/AssetManager.hpp"

#include "SHE/Core/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <string_view>
#include <utility>

namespace she
{
namespace
{
[[nodiscard]] std::string ToLowerCopy(std::string_view value)
{
    std::string normalized(value);
    std::transform(
        normalized.begin(),
        normalized.end(),
        normalized.begin(),
        [](const unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    return normalized;
}

[[nodiscard]] std::string NormalizeExtension(std::string_view extension)
{
    if (extension.empty())
    {
        return {};
    }

    std::string normalized = ToLowerCopy(extension);
    if (!normalized.empty() && normalized.front() != '.')
    {
        normalized.insert(normalized.begin(), '.');
    }

    return normalized;
}

[[nodiscard]] std::string ExtractExtension(const std::string_view sourcePath)
{
    return NormalizeExtension(std::filesystem::path(sourcePath).extension().string());
}

[[nodiscard]] std::string InferAssetType(const std::string_view logicalName)
{
    if (logicalName.starts_with("textures/"))
    {
        return "texture";
    }

    if (logicalName.starts_with("audio/"))
    {
        return "audio";
    }

    if (logicalName.starts_with("fonts/"))
    {
        return "font";
    }

    return "generic";
}

[[nodiscard]] std::string JoinStrings(const std::vector<std::string>& values)
{
    if (values.empty())
    {
        return "<none>";
    }

    std::ostringstream stream;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        stream << values[index];
        if (index + 1 < values.size())
        {
            stream << ", ";
        }
    }

    return stream.str();
}

[[nodiscard]] std::string JoinProperties(const std::vector<AssetMetadataProperty>& properties)
{
    if (properties.empty())
    {
        return "<none>";
    }

    std::ostringstream stream;
    for (std::size_t index = 0; index < properties.size(); ++index)
    {
        stream << properties[index].key << '=' << properties[index].value;
        if (index + 1 < properties.size())
        {
            stream << ", ";
        }
    }

    return stream.str();
}

void NormalizeTags(std::vector<std::string>& tags)
{
    for (auto& tag : tags)
    {
        tag = ToLowerCopy(tag);
    }

    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
}

void NormalizeProperties(std::vector<AssetMetadataProperty>& properties)
{
    for (auto& property : properties)
    {
        property.key = ToLowerCopy(property.key);
    }

    std::sort(
        properties.begin(),
        properties.end(),
        [](const AssetMetadataProperty& left, const AssetMetadataProperty& right)
        {
            return left.key < right.key;
        });
}

[[nodiscard]] AssetMetadata NormalizeMetadata(AssetMetadata metadata)
{
    metadata.assetType = metadata.assetType.empty() ? InferAssetType(metadata.logicalName) : ToLowerCopy(metadata.assetType);
    metadata.loaderKey = ToLowerCopy(metadata.loaderKey);
    metadata.owningSystem = metadata.owningSystem.empty() ? "runtime" : metadata.owningSystem;
    NormalizeTags(metadata.tags);
    NormalizeProperties(metadata.properties);
    return metadata;
}

[[nodiscard]] AssetLoaderDescriptor NormalizeLoaderDescriptor(AssetLoaderDescriptor descriptor)
{
    descriptor.loaderKey = ToLowerCopy(descriptor.loaderKey);
    descriptor.assetType = descriptor.assetType.empty() ? "generic" : ToLowerCopy(descriptor.assetType);

    std::vector<std::string> normalizedExtensions;
    normalizedExtensions.reserve(descriptor.supportedExtensions.size());
    for (const auto& extension : descriptor.supportedExtensions)
    {
        const std::string normalized = NormalizeExtension(extension);
        if (normalized.empty() ||
            std::find(normalizedExtensions.begin(), normalizedExtensions.end(), normalized) != normalizedExtensions.end())
        {
            continue;
        }

        normalizedExtensions.push_back(normalized);
    }

    descriptor.supportedExtensions = std::move(normalizedExtensions);
    return descriptor;
}

[[nodiscard]] bool LoaderSupportsExtension(
    const AssetLoaderDescriptor& descriptor,
    const std::string_view extension)
{
    if (extension.empty())
    {
        return descriptor.supportedExtensions.empty();
    }

    return std::find(
               descriptor.supportedExtensions.begin(),
               descriptor.supportedExtensions.end(),
               extension) != descriptor.supportedExtensions.end();
}

[[nodiscard]] std::optional<AssetLoaderDescriptor> ResolveLoaderForMetadata(
    const AssetMetadata& metadata,
    const std::map<std::string, AssetLoaderDescriptor>& loaders)
{
    if (!metadata.loaderKey.empty())
    {
        const auto explicitLoader = loaders.find(metadata.loaderKey);
        if (explicitLoader != loaders.end())
        {
            return explicitLoader->second;
        }

        return std::nullopt;
    }

    const std::string extension = ExtractExtension(metadata.sourcePath);
    const AssetLoaderDescriptor* extensionFallback = nullptr;
    const AssetLoaderDescriptor* typeFallback = nullptr;

    for (const auto& [loaderKey, descriptor] : loaders)
    {
        const bool typeMatches = descriptor.assetType == metadata.assetType || descriptor.assetType == "generic";
        const bool extensionMatches = LoaderSupportsExtension(descriptor, extension);

        if (typeMatches && extensionMatches)
        {
            return descriptor;
        }

        if (extensionFallback == nullptr && extensionMatches)
        {
            extensionFallback = &descriptor;
        }

        if (typeFallback == nullptr && typeMatches && descriptor.supportedExtensions.empty())
        {
            typeFallback = &descriptor;
        }
    }

    if (extensionFallback != nullptr)
    {
        return *extensionFallback;
    }

    if (typeFallback != nullptr)
    {
        return *typeFallback;
    }

    return std::nullopt;
}
} // namespace

void AssetManager::Initialize()
{
    m_nextAssetId = 1;
    m_nextLeaseId = 1;
    m_assetsByName.clear();
    m_assetNamesById.clear();
    m_loadersByKey.clear();
    m_liveLeases.clear();
    m_liveHandleCounts.clear();
    SHE_LOG_INFO("Assets", "Asset registry initialized.");
}

void AssetManager::Shutdown()
{
    SHE_LOG_INFO("Assets", "Asset registry shutdown complete.");
    m_liveHandleCounts.clear();
    m_liveLeases.clear();
    m_loadersByKey.clear();
    m_assetNamesById.clear();
    m_assetsByName.clear();
}

AssetId AssetManager::RegisterAsset(std::string logicalName, std::string sourcePath)
{
    return RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            std::move(logicalName),
            std::move(sourcePath),
            {},
            {},
            {},
            {},
            {},
            {}});
}

AssetId AssetManager::RegisterAsset(AssetMetadata metadata)
{
    if (metadata.logicalName.empty())
    {
        SHE_LOG_WARNING("Assets", "Ignored asset registration because the logical name was empty.");
        return kInvalidAssetId;
    }

    metadata = NormalizeMetadata(std::move(metadata));
    const std::string logicalName = metadata.logicalName;

    const auto existingAsset = m_assetsByName.find(logicalName);
    if (existingAsset != m_assetsByName.end())
    {
        metadata.assetId = existingAsset->second.assetId;
        existingAsset->second = std::move(metadata);
        m_assetNamesById[existingAsset->second.assetId] = existingAsset->second.logicalName;
        return existingAsset->second.assetId;
    }

    const AssetId assignedId = m_nextAssetId++;
    metadata.assetId = assignedId;
    m_assetNamesById.emplace(assignedId, logicalName);
    m_assetsByName.emplace(logicalName, std::move(metadata));
    return assignedId;
}

void AssetManager::RegisterLoader(AssetLoaderDescriptor descriptor)
{
    descriptor = NormalizeLoaderDescriptor(std::move(descriptor));
    if (descriptor.loaderKey.empty())
    {
        SHE_LOG_WARNING("Assets", "Ignored loader registration because the loader key was empty.");
        return;
    }

    m_loadersByKey.insert_or_assign(descriptor.loaderKey, std::move(descriptor));
}

bool AssetManager::HasAsset(const std::string_view logicalName) const
{
    return m_assetsByName.contains(std::string(logicalName));
}

bool AssetManager::HasLoader(const std::string_view loaderKey) const
{
    return m_loadersByKey.contains(ToLowerCopy(loaderKey));
}

AssetId AssetManager::FindAssetId(const std::string_view logicalName) const
{
    const auto asset = m_assetsByName.find(std::string(logicalName));
    if (asset == m_assetsByName.end())
    {
        return kInvalidAssetId;
    }

    return asset->second.assetId;
}

std::optional<AssetMetadata> AssetManager::FindAssetMetadata(const AssetId assetId) const
{
    const auto assetName = m_assetNamesById.find(assetId);
    if (assetName == m_assetNamesById.end())
    {
        return std::nullopt;
    }

    const auto asset = m_assetsByName.find(assetName->second);
    if (asset == m_assetsByName.end())
    {
        return std::nullopt;
    }

    return asset->second;
}

std::optional<AssetLoaderDescriptor> AssetManager::ResolveLoader(const AssetId assetId) const
{
    const auto metadata = FindAssetMetadata(assetId);
    if (!metadata.has_value())
    {
        return std::nullopt;
    }

    return ResolveLoaderForMetadata(*metadata, m_loadersByKey);
}

AssetHandle AssetManager::AcquireAsset(const AssetId assetId)
{
    if (!m_assetNamesById.contains(assetId))
    {
        return {};
    }

    const AssetHandle handle{assetId, m_nextLeaseId++};
    m_liveLeases.emplace(handle.leaseId, assetId);
    ++m_liveHandleCounts[assetId];
    return handle;
}

void AssetManager::ReleaseAsset(const AssetHandle handle)
{
    if (!handle.IsValid())
    {
        return;
    }

    const auto liveLease = m_liveLeases.find(handle.leaseId);
    if (liveLease == m_liveLeases.end() || liveLease->second != handle.assetId)
    {
        return;
    }

    m_liveLeases.erase(liveLease);

    const auto liveHandleCount = m_liveHandleCounts.find(handle.assetId);
    if (liveHandleCount == m_liveHandleCounts.end())
    {
        return;
    }

    if (liveHandleCount->second <= 1)
    {
        m_liveHandleCounts.erase(liveHandleCount);
        return;
    }

    --liveHandleCount->second;
}

std::size_t AssetManager::GetLiveHandleCount(const AssetId assetId) const
{
    const auto liveHandleCount = m_liveHandleCounts.find(assetId);
    if (liveHandleCount == m_liveHandleCounts.end())
    {
        return 0;
    }

    return liveHandleCount->second;
}

std::vector<AssetRegistryEntry> AssetManager::ListAssets() const
{
    std::vector<AssetRegistryEntry> entries;
    entries.reserve(m_assetsByName.size());

    for (const auto& [logicalName, metadata] : m_assetsByName)
    {
        const auto resolvedLoader = ResolveLoaderForMetadata(metadata, m_loadersByKey);
        entries.push_back(
            AssetRegistryEntry{
                metadata.assetId,
                metadata.logicalName,
                metadata.sourcePath,
                metadata.assetType,
                metadata.loaderKey,
                resolvedLoader.has_value() ? resolvedLoader->loaderKey : "",
                metadata.owningSystem,
                metadata.sourceRecordId,
                metadata.tags,
                metadata.properties,
                GetLiveHandleCount(metadata.assetId)});
    }

    return entries;
}

std::vector<AssetLoaderDescriptor> AssetManager::ListLoaders() const
{
    std::vector<AssetLoaderDescriptor> descriptors;
    descriptors.reserve(m_loadersByKey.size());

    for (const auto& [loaderKey, descriptor] : m_loadersByKey)
    {
        descriptors.push_back(descriptor);
    }

    return descriptors;
}

std::string AssetManager::DescribeRegistry() const
{
    if (m_assetsByName.empty())
    {
        return "(no assets registered)\n";
    }

    std::ostringstream stream;
    for (const auto& entry : ListAssets())
    {
        stream << "- " << entry.logicalName
               << ": id=" << entry.assetId
               << " | type=" << entry.assetType
               << " | source=" << (entry.sourcePath.empty() ? "<memory>" : entry.sourcePath)
               << " | declared_loader=" << (entry.declaredLoaderKey.empty() ? "<auto>" : entry.declaredLoaderKey)
               << " | resolved_loader=" << (entry.resolvedLoaderKey.empty() ? "<unresolved>" : entry.resolvedLoaderKey)
               << " | live_handles=" << entry.liveHandleCount
               << " | owner=" << entry.owningSystem;

        if (!entry.sourceRecordId.empty())
        {
            stream << " | record=" << entry.sourceRecordId;
        }

        stream << " | tags=" << JoinStrings(entry.tags)
               << " | properties=" << JoinProperties(entry.properties)
               << '\n';
    }

    return stream.str();
}

std::string AssetManager::DescribeLoaders() const
{
    if (m_loadersByKey.empty())
    {
        return "(no asset loaders registered)\n";
    }

    std::ostringstream stream;
    for (const auto& [loaderKey, descriptor] : m_loadersByKey)
    {
        stream << "- " << descriptor.loaderKey
               << ": type=" << descriptor.assetType
               << " | extensions=" << JoinStrings(descriptor.supportedExtensions)
               << " | description=" << (descriptor.description.empty() ? "<none>" : descriptor.description)
               << '\n';
    }

    return stream.str();
}

std::size_t AssetManager::GetAssetCount() const
{
    return m_assetsByName.size();
}
} // namespace she
