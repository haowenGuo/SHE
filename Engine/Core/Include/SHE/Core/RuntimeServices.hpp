#pragma once

#include "SHE/Core/ApplicationConfig.hpp"
#include "SHE/Core/Types.hpp"

#include <functional>
#include <memory>
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

class IWindowService
{
public:
    virtual ~IWindowService() = default;

    virtual void Initialize(const ApplicationConfig& config) = 0;
    virtual void PumpEvents() = 0;
    virtual bool ShouldClose() const = 0;
    virtual void RequestClose() = 0;
    virtual void Shutdown() = 0;
};

class IAssetService
{
public:
    virtual ~IAssetService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual AssetId RegisterAsset(std::string logicalName, std::string sourcePath) = 0;
    virtual bool HasAsset(std::string_view logicalName) const = 0;
    virtual std::size_t GetAssetCount() const = 0;
};

class ISceneService
{
public:
    virtual ~ISceneService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void SetActiveScene(std::string sceneName) = 0;
    virtual std::string_view GetActiveSceneName() const = 0;
    virtual EntityId CreateEntity(std::string entityName) = 0;
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
    virtual void SubmitSceneSnapshot(std::string_view sceneName, std::size_t entityCount) = 0;
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

class IScriptingService
{
public:
    virtual ~IScriptingService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void RegisterScriptModule(std::string moduleName, std::string description) = 0;
    virtual void Update(double deltaSeconds) = 0;
    virtual std::size_t GetModuleCount() const = 0;
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
