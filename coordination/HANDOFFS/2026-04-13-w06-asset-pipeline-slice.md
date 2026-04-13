# W06 Asset Pipeline Handoff

## 1. Summary Of What Changed

W06 promoted `Engine/Assets` from a name-to-path map into a first-pass asset
contract layer.

Delivered in this slice:

- stable `AssetId`-backed metadata registration
- structured asset registry queries and text export
- loader registration plus deterministic loader lookup
- lease-style `AssetHandle` lifetime tracking
- asset registry export added to the AI authoring context
- bootstrap feature metadata wired onto the new contract
- focused asset contract tests
- runtime smoke coverage for registry, loader lookup, and handle release

This slice intentionally stops short of typed renderer/audio resource objects.
The new handle is a stable lease token over asset identity and lifetime, not a
texture buffer, sound voice, or backend-specific payload.

## 2. Changed Files

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
- `Engine/Assets/Include/SHE/Assets/AssetManager.hpp`
- `Engine/Assets/Source/AssetManager.cpp`
- `Engine/AI/Source/AuthoringAiService.cpp`
- `Game/Features/Bootstrap/BootstrapFeatureLayer.cpp`
- `Tests/CMakeLists.txt`
- `Tests/Source/AssetContractTests.cpp`
- `Tests/Source/SmokeTests.cpp`
- `docs/AI_CONTEXT.md`

## 3. Changed Interfaces

Shared interface change:

- `IAssetService` in `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`

New public asset contract types:

- `AssetMetadataProperty`
- `AssetMetadata`
- `AssetLoaderDescriptor`
- `AssetHandle`
- `AssetRegistryEntry`

New or changed `IAssetService` methods:

- `RegisterAsset(AssetMetadata)`
- `RegisterLoader(AssetLoaderDescriptor)`
- `HasLoader(std::string_view)`
- `FindAssetId(std::string_view)`
- `FindAssetMetadata(AssetId)`
- `ResolveLoader(AssetId)`
- `AcquireAsset(AssetId)`
- `ReleaseAsset(AssetHandle)`
- `GetLiveHandleCount(AssetId)`
- `ListAssets()`
- `ListLoaders()`
- `DescribeRegistry()`
- `DescribeLoaders()`

Existing compatibility path retained:

- `RegisterAsset(std::string logicalName, std::string sourcePath)` still works
  and now infers a minimal asset type from the logical-name prefix.

## 4. New Asset Handle Invariants

1. `logicalName` is the stable registry key; re-registering the same logical
   asset refreshes metadata but preserves the existing `AssetId`.
2. `AssetHandle` is a lease token identified by `{ assetId, leaseId }`.
3. `AcquireAsset()` increments a per-asset live-handle count only for known
   assets; unknown ids return an invalid handle.
4. `ReleaseAsset()` is idempotent for stale or duplicate releases. A second
   release of the same handle is ignored rather than underflowing the count.
5. `Shutdown()` clears loader state, registry state, and live leases. Any
   previously held handles should be treated as invalid after shutdown.

## 5. Loader Resolution Rules

Resolution order is:

1. explicit `metadata.loaderKey` if present and registered
2. first loader that matches both `assetType` and file extension
3. first loader that matches file extension
4. first loader that matches `assetType` and accepts extensionless assets

Current loader descriptors are metadata only. They do not instantiate backend
resources or own import/cooking behavior yet.

## 6. Tests Run And Results

Configured:

- `cmake --preset default`

Built:

- `cmake --build --preset build-default`

Tested:

- `ctest --preset test-default`

Result:

- `she_smoke_tests` passed
- `she_gameplay_contract_tests` passed
- `she_data_contract_tests` passed
- `she_asset_contract_tests` passed

## 7. Downstream Assumptions Introduced

1. W08/W10 can depend on `AssetId`, `AssetMetadata`, loader descriptors, and
   lease-style handles without depending on `AssetManager` internals.
2. W08/W10 should treat `AssetHandle` as a lifetime token around registry
   ownership, then layer renderer/audio-specific resource views on top.
3. Codex-facing runtime inspection can now consume `[asset_registry]` from the
   AI authoring context instead of scraping feature code for asset assumptions.
4. This slice does not freeze residency states, async streaming, import
   pipelines, or backend payload shapes.
5. W05 does not need to expose world-owned renderer/audio resource objects for
   this contract to stay valid.

## 8. Exact Next Steps For The Next Codex

1. Report the shared `IAssetService` contract change to W00 for integration
   review.
2. Let W08 and W10 consume `AssetMetadata` plus `ResolveLoader()` before adding
   any typed asset payload APIs.
3. Extend the contract with residency/loading state only when a real runtime
   consumer proves the need.
4. If AI/runtime context needs asset inspection later, prefer consuming
   `ListAssets()` and `DescribeRegistry()` rather than scraping feature files.

## 9. Current Status Recommendation

`ready_for_integration`
