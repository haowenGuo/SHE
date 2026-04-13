#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <map>

struct SDL_Renderer;
struct SDL_Texture;

namespace she
{
// Renderer2DService is the first non-null renderer path. It bootstraps a
// capture-oriented 2D backend that owns frame begin/end boundaries, resolves
// materials through the asset registry, and records the submitted camera and
// sprite work for later presentation/debug consumers.
class Renderer2DService final : public IRendererService
{
public:
    explicit Renderer2DService(
        std::shared_ptr<IAssetService> assetService,
        std::shared_ptr<IWindowService> windowService = nullptr);

    void Initialize(const ApplicationConfig& config) override;
    void Shutdown() override;
    void BeginFrame(FrameIndex frameIndex) override;
    void RegisterMaterial(Material2DDescriptor descriptor) override;
    [[nodiscard]] bool HasMaterial(std::string_view materialName) const override;
    [[nodiscard]] std::optional<ResolvedMaterial2D> ResolveMaterial(std::string_view materialName) const override;
    [[nodiscard]] bool SubmitCamera(const Camera2DSubmission& camera) override;
    [[nodiscard]] bool SubmitSprite(const Sprite2DSubmission& sprite) override;
    void SubmitSceneSnapshot(std::string_view sceneName, std::size_t entityCount) override;
    [[nodiscard]] bool HasFrameInFlight() const override;
    [[nodiscard]] std::optional<RenderFrameSnapshot> GetLastCompletedFrame() const override;
    [[nodiscard]] RendererBackendInfo GetBackendInfo() const override;
    void EndFrame() override;

private:
    struct ActiveFrameState
    {
        RenderFrameSnapshot snapshot;
        std::map<AssetId, AssetHandle> textureLeases;
    };

    [[nodiscard]] static std::string NormalizeMaterialName(std::string_view materialName);
    [[nodiscard]] std::optional<ResolvedMaterial2D> ResolveMaterialRecord(std::string_view materialName) const;
    [[nodiscard]] AssetHandle AcquireFrameTextureLease(AssetId textureAssetId);
    [[nodiscard]] SDL_Texture* GetOrCreateTexture(AssetId textureAssetId);
    void DestroyTextures();
    void RenderActiveFrame();
    void ReleaseActiveFrameLeases();
    void ResetFrameSurface(const ApplicationConfig& config);

    std::shared_ptr<IAssetService> m_assetService;
    std::shared_ptr<IWindowService> m_windowService;
    RendererBackendInfo m_backendInfo{};
    std::map<std::string, Material2DDescriptor> m_materialsByName;
    std::map<AssetId, SDL_Texture*> m_texturesByAssetId;
    std::optional<ActiveFrameState> m_activeFrame;
    std::optional<RenderFrameSnapshot> m_lastCompletedFrame;
    SDL_Renderer* m_renderer = nullptr;
    bool m_initialized = false;
};
} // namespace she
