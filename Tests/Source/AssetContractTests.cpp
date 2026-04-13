#include "SHE/Assets/AssetManager.hpp"

#include <algorithm>
#include <iostream>
#include <optional>
#include <string_view>

namespace
{
bool Expect(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }

    return true;
}

bool ContainsText(const std::string& text, const std::string_view needle)
{
    return text.find(needle) != std::string::npos;
}

bool HasTag(const std::vector<std::string>& tags, const std::string_view tag)
{
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

bool HasProperty(
    const std::vector<she::AssetMetadataProperty>& properties,
    const std::string_view key,
    const std::string_view value)
{
    return std::any_of(
        properties.begin(),
        properties.end(),
        [key, value](const she::AssetMetadataProperty& property)
        {
            return property.key == key && property.value == value;
        });
}

std::optional<she::AssetRegistryEntry> FindAssetEntry(
    const std::vector<she::AssetRegistryEntry>& entries,
    const std::string_view logicalName)
{
    const auto entry = std::find_if(
        entries.begin(),
        entries.end(),
        [logicalName](const she::AssetRegistryEntry& candidate)
        {
            return candidate.logicalName == logicalName;
        });

    if (entry == entries.end())
    {
        return std::nullopt;
    }

    return *entry;
}

she::AssetMetadata BuildPlayerTextureMetadata()
{
    return she::AssetMetadata{
        she::kInvalidAssetId,
        "textures/player_idle",
        "Tests/Data/player_idle.placeholder",
        "texture",
        "",
        "Tests/Source",
        "feature.bootstrap.player_visual",
        {"hero", "sprite"},
        {
            {"usage", "player"},
            {"variant", "idle"},
        }};
}

she::AssetMetadata BuildPlayerTextureRefreshMetadata()
{
    return she::AssetMetadata{
        she::kInvalidAssetId,
        "textures/player_idle",
        "Tests/Data/player_idle.placeholder",
        "texture",
        "",
        "Tests/Source",
        "feature.bootstrap.player_visual",
        {"hero", "sprite"},
        {
            {"usage", "player"},
            {"variant", "idle"},
            {"palette", "bootstrap"},
        }};
}

she::AssetMetadata BuildPlayerFireMetadata()
{
    return she::AssetMetadata{
        she::kInvalidAssetId,
        "audio/player_fire",
        "Tests/Data/player_fire.placeholder",
        "audio",
        "",
        "Tests/Source",
        "feature.bootstrap.player_fire_sfx",
        {"combat", "sfx"},
        {
            {"trigger", "player_fire"},
            {"usage", "combat"},
        }};
}

bool TestAssetRegistryAndMetadata()
{
    she::AssetManager service;
    service.Initialize();

    bool passed = true;

    const she::AssetId textureId = service.RegisterAsset(BuildPlayerTextureMetadata());
    const she::AssetId soundId = service.RegisterAsset(BuildPlayerFireMetadata());
    const she::AssetId iconId = service.RegisterAsset(
        "textures/ui/icon",
        "Tests/Data/ui_icon.png");
    const she::AssetId refreshedTextureId = service.RegisterAsset(BuildPlayerTextureRefreshMetadata());

    passed &= Expect(textureId != she::kInvalidAssetId, "Texture asset should receive a stable asset id.");
    passed &= Expect(soundId != she::kInvalidAssetId, "Audio asset should receive a stable asset id.");
    passed &= Expect(iconId != she::kInvalidAssetId, "Legacy register-asset overload should still work.");
    passed &= Expect(
        textureId == refreshedTextureId,
        "Re-registering the same logical asset should preserve the existing asset id.");
    passed &= Expect(service.GetAssetCount() == 3, "Asset registry should contain three logical assets.");
    passed &= Expect(service.HasAsset("textures/player_idle"), "Asset registry should answer logical-name lookups.");
    passed &= Expect(
        service.FindAssetId("textures/player_idle") == textureId,
        "FindAssetId should return the stable asset id for a logical name.");

    const auto textureMetadata = service.FindAssetMetadata(textureId);
    passed &= Expect(textureMetadata.has_value(), "Asset metadata should be queryable by asset id.");
    if (textureMetadata.has_value())
    {
        passed &= Expect(textureMetadata->assetType == "texture", "Texture metadata should preserve its declared asset type.");
        passed &= Expect(
            textureMetadata->sourceRecordId == "feature.bootstrap.player_visual",
            "Texture metadata should preserve its source record identifier.");
        passed &= Expect(HasTag(textureMetadata->tags, "hero"), "Texture metadata should preserve normalized tags.");
        passed &= Expect(
            HasProperty(textureMetadata->properties, "palette", "bootstrap"),
            "Re-registered texture metadata should replace properties for the logical asset.");
    }

    const auto iconMetadata = service.FindAssetMetadata(iconId);
    passed &= Expect(iconMetadata.has_value(), "Legacy asset registrations should still expose metadata.");
    if (iconMetadata.has_value())
    {
        passed &= Expect(
            iconMetadata->assetType == "texture",
            "Legacy asset registrations should infer a stable asset type from the logical name.");
    }

    const auto entries = service.ListAssets();
    passed &= Expect(entries.size() == 3, "ListAssets should return one registry entry per logical asset.");
    const auto playerTextureEntry = FindAssetEntry(entries, "textures/player_idle");
    passed &= Expect(playerTextureEntry.has_value(), "ListAssets should contain the player texture entry.");
    if (playerTextureEntry.has_value())
    {
        passed &= Expect(
            playerTextureEntry->liveHandleCount == 0,
            "Freshly registered assets should start without active handles.");
        passed &= Expect(
            playerTextureEntry->resolvedLoaderKey.empty(),
            "Unconfigured assets should remain unresolved until loaders are registered.");
    }

    const std::string registry = service.DescribeRegistry();
    passed &= Expect(
        ContainsText(registry, "textures/player_idle"),
        "Registry description should contain registered logical names.");
    passed &= Expect(
        ContainsText(registry, "feature.bootstrap.player_fire_sfx"),
        "Registry description should contain source record identifiers.");
    passed &= Expect(
        ContainsText(registry, "palette=bootstrap"),
        "Registry description should include structured metadata properties.");

    service.Shutdown();
    return passed;
}

bool TestLoaderLookupAndHandleLifetime()
{
    she::AssetManager service;
    service.Initialize();

    bool passed = true;

    service.RegisterLoader(
        she::AssetLoaderDescriptor{
            "texture_placeholder",
            "texture",
            {".placeholder", ".png"},
            "Bootstrap placeholder loader for texture-like assets."});
    service.RegisterLoader(
        she::AssetLoaderDescriptor{
            "audio_placeholder",
            "audio",
            {".placeholder", ".wav"},
            "Bootstrap placeholder loader for audio-like assets."});

    const she::AssetId textureId = service.RegisterAsset(BuildPlayerTextureMetadata());
    const she::AssetId soundId = service.RegisterAsset(BuildPlayerFireMetadata());

    passed &= Expect(service.HasLoader("TEXTURE_PLACEHOLDER"), "Loader lookup should normalize loader keys.");

    const auto textureLoader = service.ResolveLoader(textureId);
    const auto soundLoader = service.ResolveLoader(soundId);
    passed &= Expect(textureLoader.has_value(), "Texture asset should resolve a loader after registration.");
    passed &= Expect(soundLoader.has_value(), "Audio asset should resolve a loader after registration.");
    if (textureLoader.has_value())
    {
        passed &= Expect(
            textureLoader->loaderKey == "texture_placeholder",
            "Texture asset should resolve the texture placeholder loader.");
    }
    if (soundLoader.has_value())
    {
        passed &= Expect(
            soundLoader->loaderKey == "audio_placeholder",
            "Audio asset should resolve the audio placeholder loader.");
    }

    const she::AssetHandle firstHandle = service.AcquireAsset(textureId);
    const she::AssetHandle secondHandle = service.AcquireAsset(textureId);
    const she::AssetHandle invalidHandle = service.AcquireAsset(9999);

    passed &= Expect(firstHandle.IsValid(), "AcquireAsset should return a live handle for a known asset.");
    passed &= Expect(secondHandle.IsValid(), "AcquireAsset should allow multiple handles for the same asset.");
    passed &= Expect(!invalidHandle.IsValid(), "AcquireAsset should reject unknown asset ids.");
    passed &= Expect(
        service.GetLiveHandleCount(textureId) == 2,
        "Handle acquisition should increment the per-asset live handle count.");

    service.ReleaseAsset(firstHandle);
    passed &= Expect(
        service.GetLiveHandleCount(textureId) == 1,
        "Releasing a handle should decrement the per-asset live handle count.");

    service.ReleaseAsset(firstHandle);
    passed &= Expect(
        service.GetLiveHandleCount(textureId) == 1,
        "Releasing the same handle twice should be ignored instead of underflowing counts.");

    service.ReleaseAsset(secondHandle);
    passed &= Expect(
        service.GetLiveHandleCount(textureId) == 0,
        "Releasing the final handle should clear the per-asset live handle count.");

    const auto registryAfterRelease = FindAssetEntry(service.ListAssets(), "textures/player_idle");
    passed &= Expect(
        registryAfterRelease.has_value() && registryAfterRelease->resolvedLoaderKey == "texture_placeholder",
        "Registry entries should expose the resolved loader key after loaders are registered.");

    const auto loaders = service.ListLoaders();
    passed &= Expect(loaders.size() == 2, "ListLoaders should return every registered loader descriptor.");

    const std::string loaderCatalog = service.DescribeLoaders();
    passed &= Expect(
        ContainsText(loaderCatalog, ".placeholder"),
        "Loader catalog should describe the file extensions each loader accepts.");
    passed &= Expect(
        ContainsText(loaderCatalog, "audio_placeholder"),
        "Loader catalog should include the registered audio loader.");

    service.Shutdown();
    return passed;
}
} // namespace

int main()
{
    const bool registryPassed = TestAssetRegistryAndMetadata();
    const bool loaderPassed = TestLoaderLookupAndHandleLifetime();
    return registryPassed && loaderPassed ? 0 : 1;
}
