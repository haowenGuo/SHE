#pragma once

#include "SHE/Core/ApplicationConfig.hpp"
#include "SHE/Core/Types.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace she
{
// These interfaces are the main contracts between the application loop and the
// engine modules. The goal is to let Core own the runtime sequencing while each
// module is free to evolve its internal implementation later.

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

struct RuntimeServices
{
    std::shared_ptr<IWindowService> window;
    std::shared_ptr<IAssetService> assets;
    std::shared_ptr<ISceneService> scene;
    std::shared_ptr<IRendererService> renderer;
    std::shared_ptr<IPhysicsService> physics;
    std::shared_ptr<IAudioService> audio;
    std::shared_ptr<IUiService> ui;

    [[nodiscard]] bool IsComplete() const
    {
        return window && assets && scene && renderer && physics && audio && ui;
    }
};
} // namespace she

