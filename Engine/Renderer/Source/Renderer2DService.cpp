#include "SHE/Renderer/Renderer2DService.hpp"

#include "SHE/Core/Logger.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace she
{
namespace
{
constexpr float kWorldUnitsToPixels = 64.0F;

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

[[nodiscard]] float Clamp01(const float value)
{
    return std::clamp(value, 0.0F, 1.0F);
}

[[nodiscard]] Color MultiplyColor(const Color& left, const Color& right)
{
    return Color{
        left.r * right.r,
        left.g * right.g,
        left.b * right.b,
        left.a * right.a,
    };
}

[[nodiscard]] bool NearlyEqual(const float left, const float right, const float tolerance = 0.05F)
{
    return std::fabs(left - right) <= tolerance;
}

[[nodiscard]] std::uint32_t HashValue(std::string_view value)
{
    std::uint32_t hash = 2166136261u;
    for (const unsigned char character : value)
    {
        hash ^= character;
        hash *= 16777619u;
    }

    return hash;
}

[[nodiscard]] Color BuildColorFromSeed(const std::uint32_t seed)
{
    const float red = 0.25F + static_cast<float>((seed >> 0) & 0xFFu) / 510.0F;
    const float green = 0.25F + static_cast<float>((seed >> 8) & 0xFFu) / 510.0F;
    const float blue = 0.25F + static_cast<float>((seed >> 16) & 0xFFu) / 510.0F;
    return Color{Clamp01(red), Clamp01(green), Clamp01(blue), 1.0F};
}

[[nodiscard]] SDL_FRect BuildSpriteDestination(
    const RenderedSprite2D& sprite,
    const Camera2DSubmission* camera,
    const int surfaceWidth,
    const int surfaceHeight)
{
    const Vector2 cameraCenter = camera != nullptr ? camera->worldCenter : Vector2{};
    const float zoom = camera != nullptr && camera->zoom > 0.0F ? camera->zoom : 1.0F;
    const float width = std::max(1.0F, sprite.size.x * kWorldUnitsToPixels * zoom);
    const float height = std::max(1.0F, sprite.size.y * kWorldUnitsToPixels * zoom);
    const float centerX = static_cast<float>(surfaceWidth) * 0.5F +
                          (sprite.position.x - cameraCenter.x) * kWorldUnitsToPixels * zoom;
    const float centerY = static_cast<float>(surfaceHeight) * 0.5F -
                          (sprite.position.y - cameraCenter.y) * kWorldUnitsToPixels * zoom;

    return SDL_FRect{
        centerX - width * 0.5F,
        centerY - height * 0.5F,
        width,
        height};
}

[[nodiscard]] std::size_t CountVisiblePixelSamples(SDL_Surface* surface, const Color& clearColor)
{
    if (surface == nullptr || surface->w <= 0 || surface->h <= 0)
    {
        return 0;
    }

    const int stepX = std::max(1, surface->w / 32);
    const int stepY = std::max(1, surface->h / 18);
    std::size_t visibleSamples = 0;

    for (int y = 0; y < surface->h; y += stepY)
    {
        for (int x = 0; x < surface->w; x += stepX)
        {
            float red = 0.0F;
            float green = 0.0F;
            float blue = 0.0F;
            float alpha = 0.0F;
            if (!SDL_ReadSurfacePixelFloat(surface, x, y, &red, &green, &blue, &alpha))
            {
                continue;
            }

            if (alpha > 0.01F &&
                (!NearlyEqual(red, clearColor.r) || !NearlyEqual(green, clearColor.g) ||
                 !NearlyEqual(blue, clearColor.b)))
            {
                ++visibleSamples;
            }
        }
    }

    return visibleSamples;
}
} // namespace

Renderer2DService::Renderer2DService(
    std::shared_ptr<IAssetService> assetService,
    std::shared_ptr<IWindowService> windowService)
    : m_assetService(std::move(assetService))
    , m_windowService(std::move(windowService))
{
}

void Renderer2DService::Initialize(const ApplicationConfig& config)
{
    m_materialsByName.clear();
    m_activeFrame.reset();
    m_lastCompletedFrame.reset();
    DestroyTextures();

    if (m_renderer != nullptr)
    {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }

    m_initialized = true;
    ResetFrameSurface(config);

    if (m_windowService != nullptr)
    {
        const NativeWindowHandle nativeWindow = m_windowService->GetNativeWindowHandle();
        if (nativeWindow.backend == NativeWindowBackend::Sdl3 && nativeWindow.handle != nullptr)
        {
            SDL_Window* window = static_cast<SDL_Window*>(nativeWindow.handle);
            const char* preferredBackend = config.startWindowHidden ? "software" : "opengl,software";
            m_renderer = SDL_CreateRenderer(window, preferredBackend);
            if (m_renderer == nullptr)
            {
                SHE_LOG_WARNING("Renderer", SDL_GetError());
            }
            else
            {
                const char* rendererName = SDL_GetRendererName(m_renderer);
                m_backendInfo.backendName = rendererName != nullptr ? std::string{"sdl3/"} + rendererName : "sdl3/unknown";
                static_cast<void>(SDL_SetDefaultTextureScaleMode(m_renderer, SDL_SCALEMODE_NEAREST));
                static_cast<void>(SDL_SetRenderVSync(m_renderer, config.startWindowHidden ? 0 : 1));
            }
        }
    }

    SHE_LOG_INFO("Renderer", "Renderer2DService initialized.");
}

void Renderer2DService::Shutdown()
{
    ReleaseActiveFrameLeases();
    m_activeFrame.reset();
    m_lastCompletedFrame.reset();
    DestroyTextures();

    if (m_renderer != nullptr)
    {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }

    m_materialsByName.clear();
    m_backendInfo = {};
    m_initialized = false;

    SHE_LOG_INFO("Renderer", "Renderer2DService shutdown complete.");
}

void Renderer2DService::BeginFrame(const FrameIndex frameIndex)
{
    if (!m_initialized)
    {
        SHE_LOG_WARNING("Renderer", "BeginFrame ignored because the renderer was not initialized.");
        return;
    }

    ReleaseActiveFrameLeases();
    m_activeFrame.reset();

    if (m_windowService != nullptr)
    {
        const WindowState windowState = m_windowService->GetWindowState();
        if (windowState.width > 0)
        {
            m_backendInfo.defaultSurfaceWidth = windowState.width;
        }

        if (windowState.height > 0)
        {
            m_backendInfo.defaultSurfaceHeight = windowState.height;
        }
    }

    m_activeFrame = ActiveFrameState{};
    m_activeFrame->snapshot.frameIndex = frameIndex;
    m_activeFrame->snapshot.backendName = m_backendInfo.backendName;
    m_activeFrame->snapshot.surfaceWidth = m_backendInfo.defaultSurfaceWidth;
    m_activeFrame->snapshot.surfaceHeight = m_backendInfo.defaultSurfaceHeight;
}

void Renderer2DService::RegisterMaterial(Material2DDescriptor descriptor)
{
    if (descriptor.materialName.empty())
    {
        SHE_LOG_WARNING("Renderer", "Ignored material registration because the material name was empty.");
        return;
    }

    if (descriptor.textureAssetName.empty())
    {
        descriptor.textureAssetName = descriptor.materialName;
    }

    m_materialsByName.insert_or_assign(NormalizeMaterialName(descriptor.materialName), std::move(descriptor));
}

bool Renderer2DService::HasMaterial(const std::string_view materialName) const
{
    if (materialName.empty())
    {
        return false;
    }

    if (m_materialsByName.contains(NormalizeMaterialName(materialName)))
    {
        return true;
    }

    return m_assetService != nullptr && m_assetService->HasAsset(materialName);
}

std::optional<ResolvedMaterial2D> Renderer2DService::ResolveMaterial(const std::string_view materialName) const
{
    return ResolveMaterialRecord(materialName);
}

bool Renderer2DService::SubmitCamera(const Camera2DSubmission& camera)
{
    if (!m_activeFrame.has_value())
    {
        return false;
    }

    Camera2DSubmission submittedCamera = camera;
    if (submittedCamera.zoom <= 0.0F)
    {
        submittedCamera.zoom = 1.0F;
    }

    if (submittedCamera.viewportWidth > 0)
    {
        m_activeFrame->snapshot.surfaceWidth = submittedCamera.viewportWidth;
    }

    if (submittedCamera.viewportHeight > 0)
    {
        m_activeFrame->snapshot.surfaceHeight = submittedCamera.viewportHeight;
    }

    m_activeFrame->snapshot.camera = std::move(submittedCamera);
    return true;
}

bool Renderer2DService::SubmitSprite(const Sprite2DSubmission& sprite)
{
    if (!m_activeFrame.has_value())
    {
        return false;
    }

    const auto resolvedMaterial = ResolveMaterialRecord(sprite.materialName);
    if (!resolvedMaterial.has_value())
    {
        return false;
    }

    const AssetHandle textureLease = AcquireFrameTextureLease(resolvedMaterial->textureAssetId);
    if (!textureLease.IsValid())
    {
        return false;
    }

    m_activeFrame->snapshot.sprites.push_back(
        RenderedSprite2D{
            sprite.entityId,
            sprite.entityName,
            *resolvedMaterial,
            sprite.position,
            sprite.rotationDegrees,
            sprite.size,
            MultiplyColor(resolvedMaterial->tint, sprite.tint),
            sprite.sortKey});
    return true;
}

void Renderer2DService::SubmitSceneSnapshot(const std::string_view sceneName, const std::size_t entityCount)
{
    if (!m_activeFrame.has_value())
    {
        return;
    }

    m_activeFrame->snapshot.sceneName = std::string(sceneName);
    m_activeFrame->snapshot.sceneEntityCount = entityCount;
}

bool Renderer2DService::HasFrameInFlight() const
{
    return m_activeFrame.has_value();
}

std::optional<RenderFrameSnapshot> Renderer2DService::GetLastCompletedFrame() const
{
    return m_lastCompletedFrame;
}

RendererBackendInfo Renderer2DService::GetBackendInfo() const
{
    return m_backendInfo;
}

void Renderer2DService::EndFrame()
{
    if (!m_activeFrame.has_value())
    {
        return;
    }

    std::stable_sort(
        m_activeFrame->snapshot.sprites.begin(),
        m_activeFrame->snapshot.sprites.end(),
        [](const RenderedSprite2D& left, const RenderedSprite2D& right)
        {
            if (left.sortKey == right.sortKey)
            {
                return left.entityId < right.entityId;
            }

            return left.sortKey < right.sortKey;
        });

    RenderActiveFrame();
    m_lastCompletedFrame = m_activeFrame->snapshot;
    ReleaseActiveFrameLeases();
    m_activeFrame.reset();
}

std::string Renderer2DService::NormalizeMaterialName(const std::string_view materialName)
{
    return ToLowerCopy(materialName);
}

std::optional<ResolvedMaterial2D> Renderer2DService::ResolveMaterialRecord(const std::string_view materialName) const
{
    if (materialName.empty() || m_assetService == nullptr)
    {
        return std::nullopt;
    }

    Material2DDescriptor descriptor;
    const auto registeredMaterial = m_materialsByName.find(NormalizeMaterialName(materialName));
    if (registeredMaterial != m_materialsByName.end())
    {
        descriptor = registeredMaterial->second;
    }
    else
    {
        if (!m_assetService->HasAsset(materialName))
        {
            return std::nullopt;
        }

        descriptor.materialName = std::string(materialName);
        descriptor.textureAssetName = std::string(materialName);
        descriptor.tint = {};
    }

    if (descriptor.textureAssetName.empty())
    {
        descriptor.textureAssetName = descriptor.materialName;
    }

    const AssetId textureAssetId = m_assetService->FindAssetId(descriptor.textureAssetName);
    if (textureAssetId == kInvalidAssetId)
    {
        return std::nullopt;
    }

    const auto metadata = m_assetService->FindAssetMetadata(textureAssetId);
    if (!metadata.has_value())
    {
        return std::nullopt;
    }

    const auto loader = m_assetService->ResolveLoader(textureAssetId);
    return ResolvedMaterial2D{
        descriptor.materialName,
        descriptor.textureAssetName,
        textureAssetId,
        metadata->sourcePath,
        loader.has_value() ? loader->loaderKey : "",
        descriptor.tint};
}

AssetHandle Renderer2DService::AcquireFrameTextureLease(const AssetId textureAssetId)
{
    if (!m_activeFrame.has_value() || m_assetService == nullptr)
    {
        return {};
    }

    const auto existingLease = m_activeFrame->textureLeases.find(textureAssetId);
    if (existingLease != m_activeFrame->textureLeases.end())
    {
        return existingLease->second;
    }

    const AssetHandle newLease = m_assetService->AcquireAsset(textureAssetId);
    if (newLease.IsValid())
    {
        m_activeFrame->textureLeases.emplace(textureAssetId, newLease);
    }

    return newLease;
}

SDL_Texture* Renderer2DService::GetOrCreateTexture(const AssetId textureAssetId)
{
    if (m_renderer == nullptr || m_assetService == nullptr)
    {
        return nullptr;
    }

    const auto existingTexture = m_texturesByAssetId.find(textureAssetId);
    if (existingTexture != m_texturesByAssetId.end())
    {
        return existingTexture->second;
    }

    const auto metadata = m_assetService->FindAssetMetadata(textureAssetId);
    if (!metadata.has_value())
    {
        return nullptr;
    }

    constexpr int kTextureWidth = 16;
    constexpr int kTextureHeight = 16;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kTextureWidth * kTextureHeight * 4), 0);

    const Color primaryColor = BuildColorFromSeed(HashValue(metadata->logicalName));
    const Color secondaryColor = BuildColorFromSeed(HashValue(metadata->sourcePath));

    for (int y = 0; y < kTextureHeight; ++y)
    {
        for (int x = 0; x < kTextureWidth; ++x)
        {
            const bool border = x == 0 || y == 0 || x == kTextureWidth - 1 || y == kTextureHeight - 1;
            const bool checker = ((x / 4) + (y / 4)) % 2 == 0;
            const Color pixelColor = border ? Color{0.08F, 0.08F, 0.10F, 1.0F} : (checker ? primaryColor : secondaryColor);
            const std::size_t index = static_cast<std::size_t>((y * kTextureWidth + x) * 4);
            pixels[index + 0] = static_cast<std::uint8_t>(std::lround(Clamp01(pixelColor.r) * 255.0F));
            pixels[index + 1] = static_cast<std::uint8_t>(std::lround(Clamp01(pixelColor.g) * 255.0F));
            pixels[index + 2] = static_cast<std::uint8_t>(std::lround(Clamp01(pixelColor.b) * 255.0F));
            pixels[index + 3] = static_cast<std::uint8_t>(std::lround(Clamp01(pixelColor.a) * 255.0F));
        }
    }

    SDL_Texture* texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, kTextureWidth, kTextureHeight);
    if (texture == nullptr)
    {
        SHE_LOG_WARNING("Renderer", SDL_GetError());
        return nullptr;
    }

    if (!SDL_UpdateTexture(texture, nullptr, pixels.data(), kTextureWidth * 4))
    {
        SHE_LOG_WARNING("Renderer", SDL_GetError());
        SDL_DestroyTexture(texture);
        return nullptr;
    }

    static_cast<void>(SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST));
    m_texturesByAssetId.emplace(textureAssetId, texture);
    return texture;
}

void Renderer2DService::DestroyTextures()
{
    for (const auto& [assetId, texture] : m_texturesByAssetId)
    {
        static_cast<void>(assetId);
        if (texture != nullptr)
        {
            SDL_DestroyTexture(texture);
        }
    }

    m_texturesByAssetId.clear();
}

void Renderer2DService::RenderActiveFrame()
{
    if (!m_activeFrame.has_value() || m_renderer == nullptr)
    {
        return;
    }

    const Camera2DSubmission* camera = m_activeFrame->snapshot.camera.has_value() ? &(*m_activeFrame->snapshot.camera) : nullptr;
    const Color clearColor = camera != nullptr ? camera->clearColor : Color{0.02F, 0.02F, 0.03F, 1.0F};
    static_cast<void>(SDL_SetRenderDrawColorFloat(
        m_renderer,
        Clamp01(clearColor.r),
        Clamp01(clearColor.g),
        Clamp01(clearColor.b),
        Clamp01(clearColor.a)));
    static_cast<void>(SDL_RenderClear(m_renderer));

    for (const RenderedSprite2D& sprite : m_activeFrame->snapshot.sprites)
    {
        SDL_Texture* texture = GetOrCreateTexture(sprite.material.textureAssetId);
        if (texture == nullptr)
        {
            continue;
        }

        static_cast<void>(SDL_SetTextureColorModFloat(texture, Clamp01(sprite.tint.r), Clamp01(sprite.tint.g), Clamp01(sprite.tint.b)));
        static_cast<void>(SDL_SetTextureAlphaModFloat(texture, Clamp01(sprite.tint.a)));

        const SDL_FRect destination = BuildSpriteDestination(
            sprite,
            camera,
            m_activeFrame->snapshot.surfaceWidth,
            m_activeFrame->snapshot.surfaceHeight);
        static_cast<void>(SDL_RenderTextureRotated(
            m_renderer,
            texture,
            nullptr,
            &destination,
            sprite.rotationDegrees,
            nullptr,
            SDL_FLIP_NONE));
    }

    SDL_Surface* surface = SDL_RenderReadPixels(m_renderer, nullptr);
    if (surface != nullptr)
    {
        m_activeFrame->snapshot.visiblePixelSampleCount = CountVisiblePixelSamples(surface, clearColor);
        SDL_DestroySurface(surface);
    }

    m_activeFrame->snapshot.presentedToSurface = SDL_RenderPresent(m_renderer);
}

void Renderer2DService::ReleaseActiveFrameLeases()
{
    if (!m_activeFrame.has_value() || m_assetService == nullptr)
    {
        return;
    }

    for (const auto& [textureAssetId, textureLease] : m_activeFrame->textureLeases)
    {
        static_cast<void>(textureAssetId);
        m_assetService->ReleaseAsset(textureLease);
    }

    m_activeFrame->textureLeases.clear();
}

void Renderer2DService::ResetFrameSurface(const ApplicationConfig& config)
{
    m_backendInfo.backendName = "bootstrap_2d_capture";
    m_backendInfo.defaultSurfaceWidth = config.windowWidth;
    m_backendInfo.defaultSurfaceHeight = config.windowHeight;
    m_backendInfo.supportsMaterialLookup = true;
    m_backendInfo.supportsFrameCapture = true;

    if (m_windowService != nullptr)
    {
        const WindowState windowState = m_windowService->GetWindowState();
        if (windowState.width > 0)
        {
            m_backendInfo.defaultSurfaceWidth = windowState.width;
        }

        if (windowState.height > 0)
        {
            m_backendInfo.defaultSurfaceHeight = windowState.height;
        }
    }
}
} // namespace she
