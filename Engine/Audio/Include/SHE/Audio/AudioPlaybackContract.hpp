#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace she
{
// Gameplay features should request audio through the W01 gameplay event bus
// rather than reaching around it. assetName must be the W06 logical asset
// name, not a filesystem path.
enum class AudioPlaybackKind
{
    Sound,
    Music
};

struct AudioPlaybackRequest
{
    AudioPlaybackKind kind = AudioPlaybackKind::Sound;
    std::string assetName;
    std::string channelName;
    std::string groupName;
    bool looping = false;
    float volume = 1.0F;
};

struct AudioStopRequest
{
    std::string channelName;
    std::string groupName;
    std::string assetName;
};

inline constexpr std::string_view kAudioGameplayEventCategory = "audio";
inline constexpr std::string_view kAudioPlayRequestedEvent = "PlayRequested";
inline constexpr std::string_view kAudioStopRequestedEvent = "StopRequested";

[[nodiscard]] std::string_view GetAudioPlaybackKindName(AudioPlaybackKind kind);
[[nodiscard]] std::string BuildGameplayAudioPlayPayload(const AudioPlaybackRequest& request);
[[nodiscard]] std::optional<AudioPlaybackRequest> ParseGameplayAudioPlayPayload(std::string_view payload);
[[nodiscard]] std::string BuildGameplayAudioStopPayload(const AudioStopRequest& request);
[[nodiscard]] std::optional<AudioStopRequest> ParseGameplayAudioStopPayload(std::string_view payload);
} // namespace she
