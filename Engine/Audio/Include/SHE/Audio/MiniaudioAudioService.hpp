#pragma once

#include "SHE/Audio/AudioPlaybackContract.hpp"
#include "SHE/Core/RuntimeServices.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace she
{
struct AudioRuntimeOptions
{
    bool preferPlaybackDevice = true;
};

// MiniaudioAudioService keeps the runtime playback path inside Engine/Audio and
// consumes W01/W06 contracts directly. Gameplay-facing code should queue audio
// requests through AudioPlaybackContract on the gameplay event bus.
class MiniaudioAudioService final : public IAudioService
{
public:
    MiniaudioAudioService(
        std::shared_ptr<IAssetService> assets,
        std::shared_ptr<IGameplayService> gameplay,
        AudioRuntimeOptions options = {});
    ~MiniaudioAudioService() override;

    void Initialize() override;
    void Shutdown() override;
    void Update(double deltaSeconds) override;

    [[nodiscard]] bool IsPlaybackDeviceActive() const;
    [[nodiscard]] std::size_t GetActiveVoiceCount() const;
    [[nodiscard]] std::size_t GetGroupVoiceCount(std::string_view groupName) const;
    [[nodiscard]] bool HasActiveChannel(std::string_view channelName) const;
    [[nodiscard]] std::string DescribePlaybackState() const;

private:
    struct Impl;

    void RegisterDefaultLoader() const;
    void HandleGameplayEvent(const GameplayEvent& event);
    void HandlePlayRequest(const AudioPlaybackRequest& request);
    void HandleStopRequest(const AudioStopRequest& request);
    void StopChannel(std::string_view channelName);
    void StopGroup(std::string_view groupName);
    void StopAsset(std::string_view assetName);
    void StopAllVoices();
    void PruneFinishedVoices();

    [[nodiscard]] std::string CanonicalizeGroupName(const AudioPlaybackRequest& request) const;
    [[nodiscard]] std::string CanonicalizeChannelName(const AudioPlaybackRequest& request, std::string_view groupName);
    [[nodiscard]] bool TryInitializeEngine(bool withPlaybackDevice);

    std::shared_ptr<IAssetService> m_assets;
    std::shared_ptr<IGameplayService> m_gameplay;
    AudioRuntimeOptions m_options;
    std::size_t m_gameplaySubscriptionId = 0;
    std::unique_ptr<Impl> m_impl;
};
} // namespace she
