#include <SDL3/SDL_main.h>

#include "SHE/Assets/AssetManager.hpp"
#include "SHE/Platform/SdlWindowService.hpp"
#include "SHE/Renderer/Renderer2DService.hpp"

#include <iostream>
#include <memory>
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

bool NearlyEqual(const float left, const float right)
{
    constexpr float kTolerance = 0.0001F;
    const float difference = left - right;
    return difference < kTolerance && difference > -kTolerance;
}

std::shared_ptr<she::AssetManager> CreateAssetRegistry()
{
    auto assets = std::make_shared<she::AssetManager>();
    assets->Initialize();
    assets->RegisterLoader(
        she::AssetLoaderDescriptor{
            "texture_placeholder",
            "texture",
            {".placeholder", ".png"},
            "Renderer contract placeholder texture loader."});
    return assets;
}

std::shared_ptr<she::SdlWindowService> CreateHiddenWindow()
{
    auto window = std::make_shared<she::SdlWindowService>();

    she::ApplicationConfig config;
    config.applicationName = "SHE Renderer Contract Tests";
    config.windowWidth = 960;
    config.windowHeight = 540;
    config.startWindowHidden = true;

    window->Initialize(config);
    return window;
}

she::AssetId RegisterPlayerTexture(const std::shared_ptr<she::AssetManager>& assets)
{
    return assets->RegisterAsset(
        she::AssetMetadata{
            she::kInvalidAssetId,
            "textures/player_idle",
            "Tests/Data/player_idle.placeholder",
            "texture",
            "",
            "Tests/Source",
            "tests.renderer.player_visual",
            {"hero", "sprite"},
            {
                {"usage", "player"},
                {"variant", "idle"},
            }});
}

bool TestBackendBootstrapAndMaterialLookup()
{
    auto assets = CreateAssetRegistry();
    auto window = CreateHiddenWindow();
    const she::AssetId textureId = RegisterPlayerTexture(assets);

    she::ApplicationConfig config;
    config.windowWidth = 960;
    config.windowHeight = 540;
    config.startWindowHidden = true;

    she::Renderer2DService renderer(assets, window);
    renderer.Initialize(config);
    renderer.RegisterMaterial(
        she::Material2DDescriptor{
            "materials/player",
            "textures/player_idle",
            she::Color{0.8F, 0.9F, 1.0F, 1.0F}});

    bool passed = true;

    const she::RendererBackendInfo backendInfo = renderer.GetBackendInfo();
    passed &= Expect(
        backendInfo.backendName.starts_with("sdl3/"),
        "Renderer backend should bootstrap a real SDL-backed 2D path.");
    passed &= Expect(
        backendInfo.defaultSurfaceWidth == 960 && backendInfo.defaultSurfaceHeight == 540,
        "Renderer backend should preserve the configured default surface size.");
    passed &= Expect(
        backendInfo.supportsMaterialLookup && backendInfo.supportsFrameCapture,
        "Renderer backend should advertise material lookup and frame capture support.");
    passed &= Expect(renderer.HasMaterial("materials/player"), "Renderer should retain registered material names.");

    const auto resolvedMaterial = renderer.ResolveMaterial("materials/player");
    passed &= Expect(resolvedMaterial.has_value(), "Renderer should resolve registered materials through the asset service.");
    if (resolvedMaterial.has_value())
    {
        passed &= Expect(
            resolvedMaterial->textureAssetId == textureId,
            "Resolved materials should expose the stable texture asset id.");
        passed &= Expect(
            resolvedMaterial->textureAssetName == "textures/player_idle",
            "Resolved materials should preserve the logical texture asset name.");
        passed &= Expect(
            resolvedMaterial->resolvedLoaderKey == "texture_placeholder",
            "Resolved materials should surface the loader chosen by the asset registry.");
        passed &= Expect(
            NearlyEqual(resolvedMaterial->tint.r, 0.8F) && NearlyEqual(resolvedMaterial->tint.g, 0.9F) &&
                NearlyEqual(resolvedMaterial->tint.b, 1.0F) && NearlyEqual(resolvedMaterial->tint.a, 1.0F),
            "Resolved materials should preserve the authored material tint.");
    }

    renderer.Shutdown();
    window->Shutdown();
    assets->Shutdown();
    return passed;
}

bool TestFrameCaptureAndLeaseOwnership()
{
    auto assets = CreateAssetRegistry();
    auto window = CreateHiddenWindow();
    const she::AssetId textureId = RegisterPlayerTexture(assets);

    she::ApplicationConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.startWindowHidden = true;

    window->Shutdown();
    window->Initialize(config);

    she::Renderer2DService renderer(assets, window);
    renderer.Initialize(config);
    renderer.RegisterMaterial(
        she::Material2DDescriptor{
            "materials/player",
            "textures/player_idle",
            she::Color{1.0F, 1.0F, 1.0F, 1.0F}});

    bool passed = true;

    passed &= Expect(!renderer.HasFrameInFlight(), "Renderer should start idle before BeginFrame.");
    passed &= Expect(
        !renderer.SubmitCamera(
            she::Camera2DSubmission{
                1,
                "PreFrameCamera",
                she::Vector2{0.0F, 0.0F},
                1.0F,
                640,
                360,
                she::Color{0.0F, 0.0F, 0.0F, 1.0F}}),
        "Camera submission should be rejected outside an active frame.");
    passed &= Expect(
        !renderer.SubmitSprite(
            she::Sprite2DSubmission{
                2,
                "PreFrameSprite",
                "materials/player",
                she::Vector2{0.0F, 0.0F},
                0.0F,
                she::Vector2{1.0F, 1.0F},
                she::Color{1.0F, 1.0F, 1.0F, 1.0F},
                0}),
        "Sprite submission should be rejected outside an active frame.");

    renderer.BeginFrame(7);
    renderer.SubmitSceneSnapshot("ContractScene", 3);
    passed &= Expect(renderer.HasFrameInFlight(), "BeginFrame should open renderer-owned frame scope.");
    passed &= Expect(
        renderer.SubmitCamera(
            she::Camera2DSubmission{
                10,
                "MainCamera",
                she::Vector2{2.0F, 3.0F},
                1.5F,
                640,
                360,
                she::Color{0.1F, 0.1F, 0.2F, 1.0F}}),
        "Renderer should accept a camera while a frame is open.");
    passed &= Expect(
        renderer.SubmitSprite(
            she::Sprite2DSubmission{
                20,
                "ForegroundSprite",
                "materials/player",
                she::Vector2{4.0F, 6.0F},
                15.0F,
                she::Vector2{2.0F, 3.0F},
                she::Color{1.0F, 1.0F, 1.0F, 1.0F},
                5}),
        "Renderer should accept the first sprite submission for an open frame.");
    passed &= Expect(
        renderer.SubmitSprite(
            she::Sprite2DSubmission{
                11,
                "BackgroundSprite",
                "materials/player",
                she::Vector2{1.0F, 2.0F},
                0.0F,
                she::Vector2{1.0F, 1.0F},
                she::Color{0.7F, 0.8F, 1.0F, 1.0F},
                0}),
        "Renderer should accept additional sprites that reuse the same frame-owned texture.");
    passed &= Expect(
        assets->GetLiveHandleCount(textureId) == 1,
        "Renderer should lease each texture at most once per frame even when multiple sprites use it.");

    renderer.EndFrame();

    passed &= Expect(!renderer.HasFrameInFlight(), "EndFrame should close renderer-owned frame scope.");
    passed &= Expect(
        assets->GetLiveHandleCount(textureId) == 0,
        "Renderer should release frame-owned texture leases at the frame boundary.");

    const auto frame = renderer.GetLastCompletedFrame();
    passed &= Expect(frame.has_value(), "Renderer should capture the completed frame snapshot at EndFrame.");
    if (frame.has_value())
    {
        passed &= Expect(frame->frameIndex == 7, "Completed frame snapshot should retain the submitted frame index.");
        passed &= Expect(
            frame->sceneName == "ContractScene" && frame->sceneEntityCount == 3,
            "Completed frame snapshot should retain scene metadata.");
        passed &= Expect(
            frame->surfaceWidth == 640 && frame->surfaceHeight == 360,
            "Camera submission should drive the frame surface size.");
        passed &= Expect(frame->presentedToSurface, "Completed frame should be presented through the SDL renderer.");
        passed &= Expect(
            frame->visiblePixelSampleCount > 0,
            "Completed frame should contain visible pixels beyond the camera clear color.");
        passed &= Expect(frame->camera.has_value(), "Completed frame snapshot should retain the submitted camera.");
        passed &= Expect(frame->sprites.size() == 2, "Completed frame snapshot should retain both submitted sprites.");
        if (frame->camera.has_value())
        {
            passed &= Expect(
                frame->camera->cameraName == "MainCamera" && NearlyEqual(frame->camera->zoom, 1.5F),
                "Completed frame snapshot should retain the camera data.");
        }
        if (frame->sprites.size() == 2)
        {
            passed &= Expect(
                frame->sprites[0].entityId == 11 && frame->sprites[1].entityId == 20,
                "Completed frame snapshot should sort sprites by sort key before presentation.");
            passed &= Expect(
                frame->sprites[0].material.textureAssetId == textureId &&
                    frame->sprites[1].material.resolvedLoaderKey == "texture_placeholder",
                "Completed frame snapshot should retain resolved texture/material lookup details.");
            passed &= Expect(
                NearlyEqual(frame->sprites[0].tint.r, 0.7F) && NearlyEqual(frame->sprites[0].tint.g, 0.8F) &&
                    NearlyEqual(frame->sprites[0].tint.b, 1.0F),
                "Sprite tint should survive frame capture after material lookup.");
        }
    }

    renderer.Shutdown();
    window->Shutdown();
    assets->Shutdown();
    return passed;
}
} // namespace

int main(int, char**)
{
    const bool bootstrapPassed = TestBackendBootstrapAndMaterialLookup();
    const bool framePassed = TestFrameCaptureAndLeaseOwnership();
    return bootstrapPassed && framePassed ? 0 : 1;
}
