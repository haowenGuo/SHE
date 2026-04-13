#include "SHE/Audio/MiniaudioAudioService.hpp"

#include "SHE/Audio/AudioPlaybackContract.hpp"
#include "SHE/Core/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace she
{
namespace
{
[[nodiscard]] std::string ToLowerCopy(const std::string_view value)
{
    std::string lowered(value);
    std::transform(
        lowered.begin(),
        lowered.end(),
        lowered.begin(),
        [](const unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    return lowered;
}

[[nodiscard]] std::string NormalizeGroupName(std::string_view groupName)
{
    return ToLowerCopy(groupName);
}

[[nodiscard]] float ClampVolume(const float volume)
{
    return std::clamp(volume, 0.0F, 1.0F);
}

[[nodiscard]] bool IsSupportedExtension(const std::filesystem::path& sourcePath)
{
    const std::string extension = ToLowerCopy(sourcePath.extension().string());
    return extension == ".wav" || extension == ".mp3" || extension == ".ogg" || extension == ".flac";
}

[[nodiscard]] std::filesystem::path ResolveAssetPath(const std::string_view sourcePath)
{
    std::filesystem::path path(sourcePath);
    if (path.is_absolute())
    {
        return path;
    }

    const std::filesystem::path root(SHE_PROJECT_ROOT);
    if (root.empty())
    {
        return path;
    }

    return root / path;
}

[[nodiscard]] bool VoiceHasCompleted(const ma_sound& sound)
{
    return ma_sound_at_end(&sound) == MA_TRUE && ma_sound_is_looping(&sound) == MA_FALSE;
}
} // namespace

struct MiniaudioAudioService::Impl
{
    struct ActiveVoice
    {
        AssetId assetId = kInvalidAssetId;
        AssetHandle handle{};
        AudioPlaybackKind kind = AudioPlaybackKind::Sound;
        std::string logicalName;
        std::string channelName;
        std::string groupName;
        bool looping = false;
        float volume = 1.0F;
        std::filesystem::path sourcePath;
        ma_sound sound{};
        bool initialized = false;
    };

    ma_engine engine{};
    bool engineInitialized = false;
    bool playbackDeviceActive = false;
    std::size_t nextEphemeralChannelId = 1;
    double lastDeltaSeconds = 0.0;
    std::map<std::string, std::unique_ptr<ActiveVoice>> voicesByChannel;
};

MiniaudioAudioService::MiniaudioAudioService(
    std::shared_ptr<IAssetService> assets,
    std::shared_ptr<IGameplayService> gameplay,
    AudioRuntimeOptions options)
    : m_assets(std::move(assets))
    , m_gameplay(std::move(gameplay))
    , m_options(options)
    , m_impl(std::make_unique<Impl>())
{
}

MiniaudioAudioService::~MiniaudioAudioService() = default;

void MiniaudioAudioService::Initialize()
{
    if (!m_assets || !m_gameplay)
    {
        throw std::runtime_error("MiniaudioAudioService requires asset and gameplay services.");
    }

    RegisterDefaultLoader();

    if (!TryInitializeEngine(m_options.preferPlaybackDevice))
    {
        SHE_LOG_WARNING("Audio", "Falling back to miniaudio no-device mode because live playback initialization failed.");
        if (!TryInitializeEngine(false))
        {
            throw std::runtime_error("MiniaudioAudioService failed to initialize both live-device and no-device paths.");
        }
    }

    m_gameplaySubscriptionId = m_gameplay->SubscribeToEvent(
        std::string(kAudioGameplayEventCategory),
        {},
        [this](const GameplayEvent& event)
        {
            HandleGameplayEvent(event);
        });

    SHE_LOG_INFO("Audio", "Miniaudio audio service initialized.");
}

void MiniaudioAudioService::Shutdown()
{
    if (m_gameplaySubscriptionId != 0 && m_gameplay)
    {
        m_gameplay->UnsubscribeFromEvent(m_gameplaySubscriptionId);
        m_gameplaySubscriptionId = 0;
    }

    StopAllVoices();

    if (m_impl->engineInitialized)
    {
        ma_engine_uninit(&m_impl->engine);
        m_impl->engineInitialized = false;
    }

    m_impl->playbackDeviceActive = false;
    m_impl->lastDeltaSeconds = 0.0;
    m_impl->nextEphemeralChannelId = 1;

    SHE_LOG_INFO("Audio", "Miniaudio audio service shutdown complete.");
}

void MiniaudioAudioService::Update(const double deltaSeconds)
{
    m_impl->lastDeltaSeconds = deltaSeconds;
    PruneFinishedVoices();
}

bool MiniaudioAudioService::IsPlaybackDeviceActive() const
{
    return m_impl->playbackDeviceActive;
}

std::size_t MiniaudioAudioService::GetActiveVoiceCount() const
{
    return m_impl->voicesByChannel.size();
}

std::size_t MiniaudioAudioService::GetGroupVoiceCount(const std::string_view groupName) const
{
    const std::string normalizedGroup = NormalizeGroupName(groupName);
    std::size_t count = 0;

    for (const auto& entry : m_impl->voicesByChannel)
    {
        const auto& voice = entry.second;
        if (voice->groupName == normalizedGroup)
        {
            ++count;
        }
    }

    return count;
}

bool MiniaudioAudioService::HasActiveChannel(const std::string_view channelName) const
{
    return m_impl->voicesByChannel.contains(std::string(channelName));
}

std::string MiniaudioAudioService::DescribePlaybackState() const
{
    std::ostringstream stream;
    stream << "device_active=" << (m_impl->playbackDeviceActive ? "true" : "false") << '\n';
    stream << "active_voice_count=" << m_impl->voicesByChannel.size() << '\n';
    stream << "last_delta_seconds=" << m_impl->lastDeltaSeconds;

    if (m_impl->voicesByChannel.empty())
    {
        return stream.str();
    }

    for (const auto& [channelName, voice] : m_impl->voicesByChannel)
    {
        stream << '\n'
               << "- channel=" << channelName
               << " | asset=" << voice->logicalName
               << " | group=" << voice->groupName
               << " | kind=" << GetAudioPlaybackKindName(voice->kind)
               << " | looping=" << (voice->looping ? "true" : "false")
               << " | volume=" << voice->volume;
    }

    return stream.str();
}

void MiniaudioAudioService::RegisterDefaultLoader() const
{
    m_assets->RegisterLoader(
        AssetLoaderDescriptor{
            "miniaudio_audio",
            "audio",
            {".wav", ".mp3", ".ogg", ".flac"},
            "Miniaudio-backed runtime loader for decoded or streamed audio assets."});
}

void MiniaudioAudioService::HandleGameplayEvent(const GameplayEvent& event)
{
    if (event.name == kAudioPlayRequestedEvent)
    {
        const auto request = ParseGameplayAudioPlayPayload(event.payload);
        if (!request.has_value())
        {
            SHE_LOG_WARNING("Audio", "Ignored malformed gameplay audio play request.");
            return;
        }

        HandlePlayRequest(*request);
        return;
    }

    if (event.name == kAudioStopRequestedEvent)
    {
        const auto request = ParseGameplayAudioStopPayload(event.payload);
        if (!request.has_value())
        {
            SHE_LOG_WARNING("Audio", "Ignored malformed gameplay audio stop request.");
            return;
        }

        HandleStopRequest(*request);
    }
}

void MiniaudioAudioService::HandlePlayRequest(const AudioPlaybackRequest& request)
{
    const AssetId assetId = m_assets->FindAssetId(request.assetName);
    if (assetId == kInvalidAssetId)
    {
        SHE_LOG_WARNING("Audio", "Audio play request referenced an unknown asset.");
        return;
    }

    const auto metadata = m_assets->FindAssetMetadata(assetId);
    const auto loader = m_assets->ResolveLoader(assetId);
    if (!metadata.has_value() || metadata->assetType != "audio" || !loader.has_value() || loader->assetType != "audio")
    {
        SHE_LOG_WARNING("Audio", "Audio play request referenced an asset without an audio contract.");
        return;
    }

    const std::filesystem::path resolvedPath = ResolveAssetPath(metadata->sourcePath);
    if (!IsSupportedExtension(resolvedPath) || !std::filesystem::exists(resolvedPath))
    {
        SHE_LOG_WARNING("Audio", "Audio play request referenced a missing or unsupported audio source.");
        return;
    }

    AssetHandle handle = m_assets->AcquireAsset(assetId);
    if (!handle.IsValid())
    {
        SHE_LOG_WARNING("Audio", "Audio play request could not acquire an asset handle.");
        return;
    }

    auto voice = std::make_unique<Impl::ActiveVoice>();
    voice->assetId = assetId;
    voice->handle = handle;
    voice->kind = request.kind;
    voice->logicalName = metadata->logicalName;
    voice->groupName = CanonicalizeGroupName(request);
    voice->channelName = CanonicalizeChannelName(request, voice->groupName);
    voice->looping = request.looping || request.kind == AudioPlaybackKind::Music;
    voice->volume = ClampVolume(request.volume);
    voice->sourcePath = resolvedPath;

    const ma_uint32 flags = request.kind == AudioPlaybackKind::Music ? MA_SOUND_FLAG_STREAM : 0;
    const ma_result initResult = ma_sound_init_from_file(
        &m_impl->engine,
        voice->sourcePath.string().c_str(),
        flags,
        nullptr,
        nullptr,
        &voice->sound);
    if (initResult != MA_SUCCESS)
    {
        m_assets->ReleaseAsset(handle);
        SHE_LOG_WARNING("Audio", "Miniaudio failed to create a sound from the requested asset.");
        return;
    }

    voice->initialized = true;
    ma_sound_set_looping(&voice->sound, voice->looping ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(&voice->sound, voice->volume);

    const ma_result startResult = ma_sound_start(&voice->sound);
    if (startResult != MA_SUCCESS)
    {
        ma_sound_uninit(&voice->sound);
        m_assets->ReleaseAsset(handle);
        SHE_LOG_WARNING("Audio", "Miniaudio failed to start playback for the requested asset.");
        return;
    }

    StopChannel(voice->channelName);
    m_impl->voicesByChannel.insert_or_assign(voice->channelName, std::move(voice));
}

void MiniaudioAudioService::HandleStopRequest(const AudioStopRequest& request)
{
    if (!request.channelName.empty())
    {
        StopChannel(request.channelName);
    }

    if (!request.groupName.empty())
    {
        StopGroup(request.groupName);
    }

    if (!request.assetName.empty())
    {
        StopAsset(request.assetName);
    }
}

void MiniaudioAudioService::StopChannel(const std::string_view channelName)
{
    const auto activeVoice = m_impl->voicesByChannel.find(std::string(channelName));
    if (activeVoice == m_impl->voicesByChannel.end())
    {
        return;
    }

    if (activeVoice->second->initialized)
    {
        ma_sound_stop(&activeVoice->second->sound);
        ma_sound_uninit(&activeVoice->second->sound);
    }

    m_assets->ReleaseAsset(activeVoice->second->handle);
    m_impl->voicesByChannel.erase(activeVoice);
}

void MiniaudioAudioService::StopGroup(const std::string_view groupName)
{
    std::vector<std::string> channelsToStop;
    const std::string normalizedGroup = NormalizeGroupName(groupName);

    for (const auto& [channelName, voice] : m_impl->voicesByChannel)
    {
        if (voice->groupName == normalizedGroup)
        {
            channelsToStop.push_back(channelName);
        }
    }

    for (const auto& channelName : channelsToStop)
    {
        StopChannel(channelName);
    }
}

void MiniaudioAudioService::StopAsset(const std::string_view assetName)
{
    std::vector<std::string> channelsToStop;

    for (const auto& [channelName, voice] : m_impl->voicesByChannel)
    {
        if (voice->logicalName == assetName)
        {
            channelsToStop.push_back(channelName);
        }
    }

    for (const auto& channelName : channelsToStop)
    {
        StopChannel(channelName);
    }
}

void MiniaudioAudioService::StopAllVoices()
{
    std::vector<std::string> channelsToStop;
    channelsToStop.reserve(m_impl->voicesByChannel.size());

    for (const auto& entry : m_impl->voicesByChannel)
    {
        channelsToStop.push_back(entry.first);
    }

    for (const auto& channelName : channelsToStop)
    {
        StopChannel(channelName);
    }
}

void MiniaudioAudioService::PruneFinishedVoices()
{
    std::vector<std::string> channelsToStop;

    for (const auto& [channelName, voice] : m_impl->voicesByChannel)
    {
        if (voice->initialized && VoiceHasCompleted(voice->sound))
        {
            channelsToStop.push_back(channelName);
        }
    }

    for (const auto& channelName : channelsToStop)
    {
        StopChannel(channelName);
    }
}

std::string MiniaudioAudioService::CanonicalizeGroupName(const AudioPlaybackRequest& request) const
{
    if (!request.groupName.empty())
    {
        return NormalizeGroupName(request.groupName);
    }

    return request.kind == AudioPlaybackKind::Music ? "music" : "sfx";
}

std::string MiniaudioAudioService::CanonicalizeChannelName(
    const AudioPlaybackRequest& request,
    const std::string_view groupName)
{
    if (!request.channelName.empty())
    {
        return request.channelName;
    }

    if (request.kind == AudioPlaybackKind::Music)
    {
        return std::string(groupName) + "/main";
    }

    std::ostringstream stream;
    stream << groupName << "/oneshot/" << m_impl->nextEphemeralChannelId++;
    return stream.str();
}

bool MiniaudioAudioService::TryInitializeEngine(const bool withPlaybackDevice)
{
    if (m_impl->engineInitialized)
    {
        ma_engine_uninit(&m_impl->engine);
        m_impl->engineInitialized = false;
    }

    ma_engine_config config = ma_engine_config_init();
    config.noDevice = withPlaybackDevice ? MA_FALSE : MA_TRUE;
    if (!withPlaybackDevice)
    {
        // miniaudio requires an explicit mix format when the engine runs
        // without owning a hardware playback device.
        config.channels = 2;
        config.sampleRate = 48000;
    }

    const ma_result result = ma_engine_init(&config, &m_impl->engine);
    if (result != MA_SUCCESS)
    {
        return false;
    }

    m_impl->engineInitialized = true;
    m_impl->playbackDeviceActive = withPlaybackDevice;
    return true;
}
} // namespace she
