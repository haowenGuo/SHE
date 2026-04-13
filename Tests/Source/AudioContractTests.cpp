#include "SHE/Assets/AssetManager.hpp"
#include "SHE/Audio/AudioPlaybackContract.hpp"
#include "SHE/Audio/MiniaudioAudioService.hpp"
#include "SHE/Gameplay/GameplayService.hpp"

#include <iostream>
#include <memory>
#include <string>
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

bool ContainsText(const std::string& text, const std::string_view needle)
{
    return text.find(needle) != std::string::npos;
}

she::AssetMetadata BuildAudioMetadata(
    std::string logicalName,
    std::string sourcePath,
    std::string sourceRecordId,
    std::string usage)
{
    return she::AssetMetadata{
        she::kInvalidAssetId,
        std::move(logicalName),
        std::move(sourcePath),
        "audio",
        "",
        "Tests/Source",
        std::move(sourceRecordId),
        {"audio"},
        {
            {"usage", std::move(usage)},
        }};
}

struct AudioFixture
{
    std::shared_ptr<she::AssetManager> assets = std::make_shared<she::AssetManager>();
    std::shared_ptr<she::GameplayService> gameplay = std::make_shared<she::GameplayService>();
    std::shared_ptr<she::MiniaudioAudioService> audio;

    AudioFixture()
    {
        she::AudioRuntimeOptions options;
        options.preferPlaybackDevice = false;
        audio = std::make_shared<she::MiniaudioAudioService>(assets, gameplay, options);

        assets->Initialize();
        gameplay->Initialize();
        audio->Initialize();
    }

    ~AudioFixture()
    {
        audio->Shutdown();
        gameplay->Shutdown();
        assets->Shutdown();
    }
};

bool TestChannelReplacementAndExplicitStop()
{
    AudioFixture fixture;
    bool passed = true;

    const she::AssetId fireId = fixture.assets->RegisterAsset(
        BuildAudioMetadata(
            "audio/player_fire",
            "Game/Features/Bootstrap/Data/player_fire.wav",
            "tests.audio.player_fire",
            "combat"));
    const auto fireLoader = fixture.assets->ResolveLoader(fireId);

    passed &= Expect(fireLoader.has_value(), "Audio service should register a loader that resolves real audio assets.");
    passed &= Expect(
        fireLoader.has_value() && fireLoader->loaderKey == "miniaudio_audio",
        "Expected the bootstrap fire sound to resolve through the miniaudio loader.");

    fixture.gameplay->BeginFrame(0);
    fixture.gameplay->QueueEvent(
        std::string(she::kAudioGameplayEventCategory),
        std::string(she::kAudioPlayRequestedEvent),
        she::BuildGameplayAudioPlayPayload(
            she::AudioPlaybackRequest{
                she::AudioPlaybackKind::Sound,
                "audio/player_fire",
                "sfx/player_fire",
                "sfx",
                false,
                0.80F}));
    fixture.gameplay->FlushCommands();
    fixture.audio->Update(1.0 / 60.0);

    passed &= Expect(
        fixture.audio->HasActiveChannel("sfx/player_fire"),
        "Explicit sound requests should claim the requested channel.");
    passed &= Expect(
        fixture.audio->GetActiveVoiceCount() == 1,
        "Expected one active voice after the first sound request.");
    passed &= Expect(
        fixture.assets->GetLiveHandleCount(fireId) == 1,
        "Playing a sound should hold exactly one asset handle lease for that channel.");

    fixture.gameplay->BeginFrame(1);
    fixture.gameplay->QueueEvent(
        std::string(she::kAudioGameplayEventCategory),
        std::string(she::kAudioPlayRequestedEvent),
        she::BuildGameplayAudioPlayPayload(
            she::AudioPlaybackRequest{
                she::AudioPlaybackKind::Sound,
                "audio/player_fire",
                "sfx/player_fire",
                "sfx",
                false,
                0.35F}));
    fixture.gameplay->FlushCommands();
    fixture.audio->Update(1.0 / 60.0);

    passed &= Expect(
        fixture.audio->GetActiveVoiceCount() == 1,
        "Replaying the same channel should replace the voice instead of stacking another owner.");
    passed &= Expect(
        fixture.assets->GetLiveHandleCount(fireId) == 1,
        "Channel replacement should release the old handle before installing the new voice.");
    passed &= Expect(
        ContainsText(fixture.audio->DescribePlaybackState(), "channel=sfx/player_fire"),
        "Playback summary should expose the claimed channel owner.");

    fixture.gameplay->BeginFrame(2);
    fixture.gameplay->QueueEvent(
        std::string(she::kAudioGameplayEventCategory),
        std::string(she::kAudioStopRequestedEvent),
        she::BuildGameplayAudioStopPayload(
            she::AudioStopRequest{
                "sfx/player_fire",
                {},
                {}}));
    fixture.gameplay->FlushCommands();
    fixture.audio->Update(1.0 / 60.0);

    passed &= Expect(
        !fixture.audio->HasActiveChannel("sfx/player_fire"),
        "Stop requests should release the named channel owner.");
    passed &= Expect(
        fixture.assets->GetLiveHandleCount(fireId) == 0,
        "Stopping the final voice should return the asset lease to zero.");

    return passed;
}

bool TestMusicDefaultsAndGroupStop()
{
    AudioFixture fixture;
    bool passed = true;

    const she::AssetId musicId = fixture.assets->RegisterAsset(
        BuildAudioMetadata(
            "audio/bootstrap_theme",
            "Game/Features/Bootstrap/Data/bootstrap_theme.wav",
            "tests.audio.bootstrap_theme",
            "ambient"));
    const she::AssetId fireId = fixture.assets->RegisterAsset(
        BuildAudioMetadata(
            "audio/player_fire",
            "Game/Features/Bootstrap/Data/player_fire.wav",
            "tests.audio.player_fire",
            "combat"));

    fixture.gameplay->BeginFrame(3);
    fixture.gameplay->QueueEvent(
        std::string(she::kAudioGameplayEventCategory),
        std::string(she::kAudioPlayRequestedEvent),
        she::BuildGameplayAudioPlayPayload(
            she::AudioPlaybackRequest{
                she::AudioPlaybackKind::Music,
                "audio/bootstrap_theme",
                {},
                {},
                true,
                0.60F}));
    fixture.gameplay->QueueEvent(
        std::string(she::kAudioGameplayEventCategory),
        std::string(she::kAudioPlayRequestedEvent),
        she::BuildGameplayAudioPlayPayload(
            she::AudioPlaybackRequest{
                she::AudioPlaybackKind::Sound,
                "audio/player_fire",
                {},
                {},
                false,
                0.95F}));
    fixture.gameplay->FlushCommands();
    fixture.audio->Update(1.0 / 60.0);

    const std::string state = fixture.audio->DescribePlaybackState();

    passed &= Expect(
        !fixture.audio->IsPlaybackDeviceActive(),
        "Audio contract tests should run in miniaudio no-device mode.");
    passed &= Expect(
        fixture.audio->HasActiveChannel("music/main"),
        "Music requests without a channel should claim the stable music/main owner.");
    passed &= Expect(
        fixture.audio->GetGroupVoiceCount("music") == 1,
        "Music requests without an explicit group should land in the music group.");
    passed &= Expect(
        fixture.audio->GetGroupVoiceCount("sfx") == 1,
        "Sound requests without an explicit group should land in the sfx group.");
    passed &= Expect(
        ContainsText(state, "channel=sfx/oneshot/1"),
        "Oneshot sounds without a channel should receive a generated group-owned channel.");
    passed &= Expect(
        ContainsText(state, "looping=true"),
        "Music requests should preserve looping ownership in the playback summary.");
    passed &= Expect(
        fixture.assets->GetLiveHandleCount(musicId) == 1 && fixture.assets->GetLiveHandleCount(fireId) == 1,
        "Each active channel should hold one asset lease while playback is alive.");

    fixture.gameplay->BeginFrame(4);
    fixture.gameplay->QueueEvent(
        std::string(she::kAudioGameplayEventCategory),
        std::string(she::kAudioStopRequestedEvent),
        she::BuildGameplayAudioStopPayload(
            she::AudioStopRequest{
                {},
                "music",
                {}}));
    fixture.gameplay->FlushCommands();
    fixture.audio->Update(1.0 / 60.0);

    passed &= Expect(
        !fixture.audio->HasActiveChannel("music/main"),
        "Stopping the music group should release the default music channel.");
    passed &= Expect(
        fixture.audio->GetGroupVoiceCount("music") == 0 && fixture.audio->GetGroupVoiceCount("sfx") == 1,
        "Group stop should leave unrelated sfx ownership intact.");
    passed &= Expect(
        fixture.assets->GetLiveHandleCount(musicId) == 0 && fixture.assets->GetLiveHandleCount(fireId) == 1,
        "Stopping one group should only release the leases owned by that group.");

    fixture.gameplay->BeginFrame(5);
    fixture.gameplay->QueueEvent(
        std::string(she::kAudioGameplayEventCategory),
        std::string(she::kAudioStopRequestedEvent),
        she::BuildGameplayAudioStopPayload(
            she::AudioStopRequest{
                {},
                "sfx",
                {}}));
    fixture.gameplay->FlushCommands();
    fixture.audio->Update(1.0 / 60.0);

    passed &= Expect(
        fixture.audio->GetActiveVoiceCount() == 0,
        "Stopping the remaining sfx group should clear the playback table.");
    passed &= Expect(
        fixture.assets->GetLiveHandleCount(fireId) == 0,
        "Stopping the final group should release the remaining asset lease.");

    return passed;
}
} // namespace

int main()
{
    bool success = true;
    success = TestChannelReplacementAndExplicitStop() && success;
    success = TestMusicDefaultsAndGroupStop() && success;
    return success ? 0 : 1;
}
