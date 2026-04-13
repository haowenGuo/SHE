#pragma once

#include "SHE/Core/ApplicationConfig.hpp"
#include "SHE/Core/MathTypes.hpp"
#include "SHE/Core/Types.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace she
{
// These interfaces are the main contracts between the application loop and the
// engine modules. The goal is to let Core own the runtime sequencing while each
// module is free to evolve its internal implementation later.

enum class DataIssueSeverity
{
    Error,
    Warning
};

struct DataValidationIssue
{
    DataIssueSeverity severity = DataIssueSeverity::Error;
    std::string code;
    std::string fieldPath;
    std::string message;
};

struct DataSchemaFieldContract
{
    std::string fieldName;
    std::string valueKind = "scalar";
    bool required = false;
    std::string description;
};

struct DataSchemaContract
{
    std::string schemaName;
    std::string description;
    std::string owningSystem;
    std::vector<DataSchemaFieldContract> fields;
};

struct DataSchemaRegistrationResult
{
    std::string schemaName;
    bool accepted = false;
    std::vector<DataValidationIssue> issues;
};

struct DataLoadRequest
{
    std::string schemaName;
    std::string recordId;
    std::string sourcePath;
    std::string yamlText;
};

struct DataLoadResult
{
    std::string schemaName;
    std::string recordId;
    std::string sourcePath;
    bool parsed = false;
    bool valid = false;
    std::vector<std::string> discoveredFields;
    std::vector<DataValidationIssue> issues;
};

struct DataRegistryEntry
{
    std::string recordId;
    std::string schemaName;
    std::string sourcePath;
    bool trusted = false;
    std::vector<std::string> discoveredFields;
    std::size_t issueCount = 0;
};

enum class KeyCode
{
    Unknown = 0,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,
    Escape,
    Enter,
    Space,
    Tab,
    Backspace,
    Up,
    Down,
    Left,
    Right,
    ShiftLeft,
    ShiftRight,
    ControlLeft,
    ControlRight,
    AltLeft,
    AltRight,
    Count
};

enum class PointerButton
{
    Left = 0,
    Middle,
    Right,
    X1,
    X2,
    Count
};

struct ButtonState
{
    bool isDown = false;
    bool pressed = false;
    bool released = false;
};

struct PointerState
{
    int x = 0;
    int y = 0;
    int deltaX = 0;
    int deltaY = 0;
    float wheelX = 0.0F;
    float wheelY = 0.0F;
};

struct WindowState
{
    std::uint32_t windowId = 0;
    int width = 0;
    int height = 0;
    bool hasKeyboardFocus = false;
    bool hasMouseFocus = false;
    bool minimized = false;
    bool closeRequested = false;
    bool resizedThisFrame = false;
};

enum class NativeWindowBackend
{
    None,
    Sdl3
};

struct NativeWindowHandle
{
    NativeWindowBackend backend = NativeWindowBackend::None;
    void* handle = nullptr;

    [[nodiscard]] bool IsValid() const
    {
        return backend != NativeWindowBackend::None && handle != nullptr;
    }
};

struct AssetMetadataProperty
{
    std::string key;
    std::string value;
};

struct AssetMetadata
{
    AssetId assetId = kInvalidAssetId;
    std::string logicalName;
    std::string sourcePath;
    std::string assetType = "generic";
    std::string loaderKey;
    std::string owningSystem;
    std::string sourceRecordId;
    std::vector<std::string> tags;
    std::vector<AssetMetadataProperty> properties;
};

struct AssetLoaderDescriptor
{
    std::string loaderKey;
    std::string assetType = "generic";
    std::vector<std::string> supportedExtensions;
    std::string description;
};

struct AssetHandle
{
    AssetId assetId = kInvalidAssetId;
    std::size_t leaseId = 0;

    [[nodiscard]] bool IsValid() const
    {
        return assetId != kInvalidAssetId && leaseId != 0;
    }
};

struct AssetRegistryEntry
{
    AssetId assetId = kInvalidAssetId;
    std::string logicalName;
    std::string sourcePath;
    std::string assetType = "generic";
    std::string declaredLoaderKey;
    std::string resolvedLoaderKey;
    std::string owningSystem;
    std::string sourceRecordId;
    std::vector<std::string> tags;
    std::vector<AssetMetadataProperty> properties;
    std::size_t liveHandleCount = 0;
};

class IWindowService
{
public:
    virtual ~IWindowService() = default;

    virtual void Initialize(const ApplicationConfig& config) = 0;
    virtual void PumpEvents() = 0;
    virtual bool ShouldClose() const = 0;
    virtual void RequestClose() = 0;
    virtual ButtonState GetKeyState(KeyCode key) const = 0;
    virtual ButtonState GetPointerButtonState(PointerButton button) const = 0;
    virtual PointerState GetPointerState() const = 0;
    virtual WindowState GetWindowState() const = 0;
    virtual NativeWindowHandle GetNativeWindowHandle() const = 0;
    virtual std::size_t GetPumpCount() const = 0;
    virtual void Shutdown() = 0;
};

class IAssetService
{
public:
    virtual ~IAssetService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual AssetId RegisterAsset(std::string logicalName, std::string sourcePath) = 0;
    virtual AssetId RegisterAsset(AssetMetadata metadata) = 0;
    virtual void RegisterLoader(AssetLoaderDescriptor descriptor) = 0;
    virtual bool HasAsset(std::string_view logicalName) const = 0;
    virtual bool HasLoader(std::string_view loaderKey) const = 0;
    virtual AssetId FindAssetId(std::string_view logicalName) const = 0;
    virtual std::optional<AssetMetadata> FindAssetMetadata(AssetId assetId) const = 0;
    virtual std::optional<AssetLoaderDescriptor> ResolveLoader(AssetId assetId) const = 0;
    // AssetHandle is a lease token rather than a typed resource view. W08/W10
    // can build renderer/audio-specific wrappers on top without changing the
    // stable asset identity and lifetime rules.
    virtual AssetHandle AcquireAsset(AssetId assetId) = 0;
    virtual void ReleaseAsset(AssetHandle handle) = 0;
    virtual std::size_t GetLiveHandleCount(AssetId assetId) const = 0;
    virtual std::vector<AssetRegistryEntry> ListAssets() const = 0;
    virtual std::vector<AssetLoaderDescriptor> ListLoaders() const = 0;
    virtual std::string DescribeRegistry() const = 0;
    virtual std::string DescribeLoaders() const = 0;
    virtual std::size_t GetAssetCount() const = 0;
};

struct SceneTransform
{
    Vector2 position{};
    float rotationDegrees = 0.0F;
    Vector2 scale{1.0F, 1.0F};
};

struct SceneEntityView
{
    EntityId entityId = kInvalidEntityId;
    std::string entityName;
    SceneTransform transform;
};

struct Camera2DSubmission
{
    EntityId entityId = kInvalidEntityId;
    std::string cameraName;
    Vector2 worldCenter{};
    float zoom = 1.0F;
    int viewportWidth = 0;
    int viewportHeight = 0;
    Color clearColor{};
};

struct Sprite2DSubmission
{
    EntityId entityId = kInvalidEntityId;
    std::string entityName;
    std::string materialName;
    Vector2 position{};
    float rotationDegrees = 0.0F;
    Vector2 size{1.0F, 1.0F};
    Color tint{};
    int sortKey = 0;
};

struct Material2DDescriptor
{
    std::string materialName;
    std::string textureAssetName;
    Color tint{};
};

struct ResolvedMaterial2D
{
    std::string materialName;
    std::string textureAssetName;
    AssetId textureAssetId = kInvalidAssetId;
    std::string textureSourcePath;
    std::string resolvedLoaderKey;
    Color tint{};
};

struct RenderedSprite2D
{
    EntityId entityId = kInvalidEntityId;
    std::string entityName;
    ResolvedMaterial2D material;
    Vector2 position{};
    float rotationDegrees = 0.0F;
    Vector2 size{1.0F, 1.0F};
    Color tint{};
    int sortKey = 0;
};

struct RendererBackendInfo
{
    std::string backendName;
    int defaultSurfaceWidth = 0;
    int defaultSurfaceHeight = 0;
    bool supportsMaterialLookup = false;
    bool supportsFrameCapture = false;
};

struct RenderFrameSnapshot
{
    FrameIndex frameIndex = 0;
    std::string backendName;
    std::string sceneName;
    std::size_t sceneEntityCount = 0;
    int surfaceWidth = 0;
    int surfaceHeight = 0;
    std::optional<Camera2DSubmission> camera;
    std::vector<RenderedSprite2D> sprites;
    bool presentedToSurface = false;
    std::size_t visiblePixelSampleCount = 0;
};

using PhysicsColliderId = std::uint64_t;

constexpr PhysicsColliderId kInvalidPhysicsColliderId = 0;

enum class PhysicsBodyType
{
    Static,
    Kinematic,
    Dynamic
};

struct PhysicsBodyDefinition
{
    PhysicsBodyType bodyType = PhysicsBodyType::Static;
    Vector2 linearVelocity{};
    float angularVelocityDegrees = 0.0F;
    float linearDamping = 0.0F;
    float angularDamping = 0.0F;
    float gravityScale = 1.0F;
    bool fixedRotation = false;
    bool isBullet = false;
    bool allowSleep = true;
};

struct PhysicsBoxColliderDefinition
{
    Vector2 halfExtents{0.5F, 0.5F};
    Vector2 offset{};
    float rotationDegrees = 0.0F;
    float density = 1.0F;
    float friction = 0.2F;
    float restitution = 0.0F;
    bool isSensor = false;
};

class ISceneService
{
public:
    virtual ~ISceneService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    // Switching scenes replaces the active world contents for the service.
    virtual void SetActiveScene(std::string sceneName) = 0;
    virtual std::string_view GetActiveSceneName() const = 0;
    virtual EntityId CreateEntity(std::string entityName) = 0;
    // DestroyEntity invalidates the handle immediately. UpdateSceneGraph prunes
    // the backing storage at the scene boundary before presentation.
    virtual bool DestroyEntity(EntityId entityId) = 0;
    virtual bool HasEntity(EntityId entityId) const = 0;
    virtual bool TryGetEntityView(EntityId entityId, SceneEntityView& outEntityView) const = 0;
    virtual bool TrySetEntityTransform(EntityId entityId, const SceneTransform& transform) = 0;
    virtual std::vector<SceneEntityView> ListEntities() const = 0;
    virtual std::size_t GetEntityCount() const = 0;
    virtual void UpdateSceneGraph(double deltaSeconds) = 0;
};

class IRendererService
{
public:
    virtual ~IRendererService() = default;

    virtual void Initialize(const ApplicationConfig& config) = 0;
    virtual void Shutdown() = 0;
    virtual void BeginFrame(FrameIndex frameIndex) = 0;
    virtual void RegisterMaterial(Material2DDescriptor descriptor) = 0;
    virtual bool HasMaterial(std::string_view materialName) const = 0;
    virtual std::optional<ResolvedMaterial2D> ResolveMaterial(std::string_view materialName) const = 0;
    virtual bool SubmitCamera(const Camera2DSubmission& camera) = 0;
    virtual bool SubmitSprite(const Sprite2DSubmission& sprite) = 0;
    virtual void SubmitSceneSnapshot(std::string_view sceneName, std::size_t entityCount) = 0;
    virtual bool HasFrameInFlight() const = 0;
    virtual std::optional<RenderFrameSnapshot> GetLastCompletedFrame() const = 0;
    virtual RendererBackendInfo GetBackendInfo() const = 0;
    virtual void EndFrame() = 0;
};

class IReflectionService
{
public:
    virtual ~IReflectionService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void RegisterType(std::string category, std::string typeName, std::string description) = 0;
    virtual void RegisterFeature(std::string featureName, std::string ownerModule, std::string description) = 0;
    virtual std::size_t GetRegisteredTypeCount() const = 0;
    virtual std::size_t GetRegisteredFeatureCount() const = 0;
    virtual std::string BuildTypeCatalog() const = 0;
};

class IDataService
{
public:
    virtual ~IDataService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual DataSchemaRegistrationResult RegisterSchema(DataSchemaContract schema) = 0;
    virtual DataLoadResult LoadRecord(const DataLoadRequest& request) = 0;
    virtual bool HasSchema(std::string_view schemaName) const = 0;
    virtual bool HasTrustedRecord(std::string_view recordId) const = 0;
    virtual std::size_t GetSchemaCount() const = 0;
    virtual std::size_t GetRecordCount() const = 0;
    virtual std::size_t GetTrustedRecordCount() const = 0;
    virtual std::vector<DataSchemaContract> ListSchemas() const = 0;
    virtual std::vector<DataRegistryEntry> ListRecords() const = 0;
    virtual std::string DescribeSchemas() const = 0;
    virtual std::string DescribeRegistry() const = 0;
};

// Gameplay commands are registered as stable authoring-facing verbs. The
// executor turns them into routed gameplay events so other systems can observe
// gameplay activity without poking at feature internals.
struct GameplayCommandDescriptor
{
    std::string name;
    std::string description;
    std::string routedEventCategory;
    std::string routedEventName;
};

// GameplayEvent is the record that moves through the gameplay event bus. The
// source field keeps the originating timer/command/API path visible to AI and
// diagnostics consumers.
struct GameplayEvent
{
    std::string category;
    std::string name;
    std::string payload;
    std::string source;
    FrameIndex frameIndex = 0;
};

using GameplayEventHandler = std::function<void(const GameplayEvent&)>;

class IGameplayService
{
public:
    virtual ~IGameplayService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    // Register commands during feature attach or service setup. Re-registering
    // a name updates its routed event mapping.
    virtual void RegisterCommand(GameplayCommandDescriptor descriptor) = 0;
    virtual bool HasCommand(std::string_view commandName) const = 0;
    // Empty category or name acts as a wildcard subscription.
    virtual std::size_t SubscribeToEvent(std::string category, std::string name, GameplayEventHandler handler) = 0;
    virtual void UnsubscribeFromEvent(std::size_t subscriptionId) = 0;
    virtual void BeginFrame(FrameIndex frameIndex) = 0;
    virtual void AdvanceFixedStep(double fixedTimeStep) = 0;
    virtual void AdvanceFrame(double deltaSeconds) = 0;
    virtual void QueueEvent(std::string category, std::string name, std::string payload) = 0;
    virtual void QueueCommand(std::string name, std::string payload) = 0;
    virtual void CreateTimer(std::string timerName, double durationSeconds, bool repeating) = 0;
    virtual void FlushCommands() = 0;
    virtual std::size_t GetPendingCommandCount() const = 0;
    virtual std::size_t GetTimerCount() const = 0;
    virtual std::string BuildGameplayDigest() const = 0;
};

class IPhysicsService
{
public:
    virtual ~IPhysicsService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    // Bodies are keyed by scene entity id so W09 can consume W05's query
    // surface instead of depending on SceneWorld storage internals.
    virtual bool CreateBody(EntityId entityId, const PhysicsBodyDefinition& definition) = 0;
    virtual bool DestroyBody(EntityId entityId) = 0;
    virtual bool HasBody(EntityId entityId) const = 0;
    virtual PhysicsColliderId CreateBoxCollider(EntityId entityId, const PhysicsBoxColliderDefinition& definition) = 0;
    virtual bool DestroyCollider(PhysicsColliderId colliderId) = 0;
    virtual void Step(double fixedTimeStep) = 0;
};

class IAudioService
{
public:
    virtual ~IAudioService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Update(double deltaSeconds) = 0;
};

class IUiService
{
public:
    virtual ~IUiService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void BeginFrame(FrameIndex frameIndex) = 0;
    virtual void EndFrame() = 0;
};

enum class ScriptModuleLoadState
{
    Registered,
    Loaded,
    Blocked
};

struct ScriptHostBindingDescriptor
{
    std::string bindingName;
    std::string owningService;
    std::string description;
};

struct ScriptFunctionContract
{
    std::string functionName;
    std::string description;
    std::string routedCommandName;
};

struct ScriptModuleContract
{
    std::string moduleName;
    std::string sourcePath;
    std::string language = "lua";
    std::string owningFeature;
    std::string description;
    bool autoLoad = true;
    std::vector<std::string> requiredBindings;
    std::vector<ScriptFunctionContract> exportedFunctions;
};

struct ScriptModuleLoadResult
{
    std::string moduleName;
    ScriptModuleLoadState state = ScriptModuleLoadState::Registered;
    bool loaded = false;
    std::vector<std::string> missingBindings;
    std::string message;
};

struct ScriptInvocationResult
{
    std::string moduleName;
    std::string functionName;
    std::string payload;
    bool accepted = false;
    bool queuedGameplayCommand = false;
    std::string queuedCommandName;
    std::string message;
};

struct ScriptModuleCatalogEntry
{
    ScriptModuleContract contract;
    ScriptModuleLoadState loadState = ScriptModuleLoadState::Registered;
    std::size_t invocationCount = 0;
    std::string lastStatusMessage;
};

class IScriptingService
{
public:
    virtual ~IScriptingService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void RegisterBinding(ScriptHostBindingDescriptor descriptor) = 0;
    virtual void RegisterScriptModule(ScriptModuleContract module) = 0;
    virtual bool HasBinding(std::string_view bindingName) const = 0;
    virtual bool HasScriptModule(std::string_view moduleName) const = 0;
    virtual ScriptModuleLoadResult LoadScriptModule(std::string_view moduleName) = 0;
    virtual ScriptInvocationResult InvokeScriptFunction(
        std::string_view moduleName,
        std::string_view functionName,
        std::string payload) = 0;
    virtual void Update(double deltaSeconds) = 0;
    virtual std::size_t GetBindingCount() const = 0;
    virtual std::size_t GetModuleCount() const = 0;
    virtual std::size_t GetLoadedModuleCount() const = 0;
    virtual std::vector<ScriptHostBindingDescriptor> ListBindings() const = 0;
    virtual std::vector<ScriptModuleCatalogEntry> ListModules() const = 0;
    virtual std::string BuildScriptCatalog() const = 0;
};

class IDiagnosticsService
{
public:
    virtual ~IDiagnosticsService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void BeginFrame(FrameIndex frameIndex) = 0;
    virtual void RecordPhase(std::string phaseName, std::string summary) = 0;
    virtual void EndFrame() = 0;
    virtual std::size_t GetCapturedFrameCount() const = 0;
    virtual std::string BuildLatestReport() const = 0;
};

class IAIService
{
public:
    virtual ~IAIService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void SetAuthoringIntent(std::string intent) = 0;
    virtual void RefreshContext(FrameIndex frameIndex) = 0;
    virtual std::size_t GetContextVersion() const = 0;
    virtual std::string ExportAuthoringContext() const = 0;
};

struct RuntimeServices
{
    std::shared_ptr<IWindowService> window;
    std::shared_ptr<IAssetService> assets;
    std::shared_ptr<ISceneService> scene;
    std::shared_ptr<IReflectionService> reflection;
    std::shared_ptr<IDataService> data;
    std::shared_ptr<IGameplayService> gameplay;
    std::shared_ptr<IRendererService> renderer;
    std::shared_ptr<IPhysicsService> physics;
    std::shared_ptr<IAudioService> audio;
    std::shared_ptr<IUiService> ui;
    std::shared_ptr<IScriptingService> scripting;
    std::shared_ptr<IDiagnosticsService> diagnostics;
    std::shared_ptr<IAIService> ai;

    [[nodiscard]] bool IsComplete() const
    {
        return window && assets && scene && reflection && data && gameplay && renderer && physics && audio && ui &&
               scripting && diagnostics && ai;
    }
};
} // namespace she
