# W10 Handoff - First Miniaudio Playback Runtime

## 1. Summary Of What Changed

W10 replaced the null audio service with a first real `miniaudio`-backed
runtime slice that stays inside the accepted W01/W06/W07 boundaries.

Delivered in this slice:

- a concrete `MiniaudioAudioService` that owns the playback engine and runtime
  voice table
- gameplay-facing audio payload helpers in `Engine/Audio` so features queue
  audio through `IGameplayService` instead of bypassing W01
- W06-aligned asset consumption that resolves logical asset names and holds
  `AssetHandle` leases for active voices instead of hard-coding source paths in
  gameplay code
- explicit channel/group ownership rules:
  - one owner per named channel
  - default `music/main` ownership for unnamed music requests
  - auto-generated `sfx/oneshot/<n>` channel ownership for unnamed one-shot
    sounds
  - stop-by-channel, stop-by-group, and stop-by-asset handling
- default audio loader registration for `.wav/.mp3/.ogg/.flac`
- bootstrap feature integration that starts music and routes encounter-spawn
  gameplay into an SFX request through the gameplay event bus
- focused audio contract tests plus smoke coverage through the new runtime
  service

## 2. Changed Files

- `Engine/Audio/CMakeLists.txt`
- `Engine/Audio/Include/SHE/Audio/AudioPlaybackContract.hpp`
- `Engine/Audio/Include/SHE/Audio/MiniaudioAudioService.hpp`
- `Engine/Audio/Source/AudioPlaybackContract.cpp`
- `Engine/Audio/Source/MiniaudioAudioService.cpp`
- `Game/Features/Bootstrap/BootstrapFeatureLayer.hpp`
- `Game/Features/Bootstrap/BootstrapFeatureLayer.cpp`
- `Game/Features/Bootstrap/Data/player_fire.wav`
- `Game/Features/Bootstrap/Data/bootstrap_theme.wav`
- `Game/Source/Main.cpp`
- `Tools/Sandbox/Source/Main.cpp`
- `Tests/CMakeLists.txt`
- `Tests/Source/SmokeTests.cpp`
- `Tests/Source/AudioContractTests.cpp`

## 3. Changed Interfaces

Shared runtime interfaces changed:

- none

`Engine/Core/Include/SHE/Core/RuntimeServices.hpp` stayed untouched in this
slice.

New public W10-local interfaces:

- `Engine/Audio/Include/SHE/Audio/AudioPlaybackContract.hpp`
  - `AudioPlaybackKind`
  - `AudioPlaybackRequest`
  - `AudioStopRequest`
  - `kAudioGameplayEventCategory`
  - `kAudioPlayRequestedEvent`
  - `kAudioStopRequestedEvent`
  - payload build/parse helpers
- `Engine/Audio/Include/SHE/Audio/MiniaudioAudioService.hpp`
  - `AudioRuntimeOptions`
  - `MiniaudioAudioService`

Integration note for W00:

- the shared interface surface stayed stable on purpose; downstream consumers
  can adopt the new audio path by including the W10-local audio headers rather
  than by taking a new `RuntimeServices.hpp` dependency

## 4. Gameplay / Asset Contract Rules Landed

1. Gameplay-triggered audio now flows through W01:
   - queue `audio/PlayRequested` or `audio/StopRequested` on
     `IGameplayService`
   - use the payload helpers from `AudioPlaybackContract.hpp`
2. Audio payloads must use W06 logical asset names such as
   `audio/player_fire`, never source file paths.
3. Active playback owns an `AssetHandle` lease until the voice is replaced,
   stopped, or shut down.
4. Named channels are exclusive owners; a second play request on the same
   channel replaces the previous voice.
5. Groups are coordination scopes, not backend objects. They exist so gameplay
   can stop related channels together without knowing miniaudio internals.

## 5. Tests Run And Results

Configured:

- `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --preset default`

Built:

- `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build --preset build-default`

Tested:

- `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe --preset test-default --output-on-failure`

Result:

- `she_smoke_tests` passed
- `she_gameplay_contract_tests` passed
- `she_data_contract_tests` passed
- `she_platform_input_tests` passed
- `she_scene_contract_tests` passed
- `she_asset_contract_tests` passed
- `she_audio_contract_tests` passed

## 6. Unresolved Risks

1. The current runtime is file-backed and synchronous per play request. It is a
   good first playable-runtime slice, but it is not yet a residency/cache
   system.
2. The no-device fallback uses a fixed `48 kHz / stereo` mix format so tests
   can run without a real desktop audio device. That keeps CI stable, but it is
   not a final authoring/runtime config surface.
3. Audio state is inspectable through `MiniaudioAudioService::DescribePlaybackState()`,
   but diagnostics and AI context do not yet export a first-class audio section.
4. `miniaudio` itself emits one upstream warning during MSVC compilation from
   fetched source; it does not fail the build, but W00 should not mistake that
   for a project-local warning regression.

## 7. Exact Next Steps For The Next Codex

1. Report this handoff to W00 and mark W10 as `ready_for_integration`.
2. Keep downstream gameplay/features on the new `audio/*Requested` gameplay
   event convention instead of adding direct feature-side miniaudio calls.
3. If W11 or W03 needs richer runtime visibility, add audio diagnostics/export
   on top of the current W10-local contract before touching shared runtime
   interfaces.
4. If later runtime work needs spatial or entity-bound audio, layer that onto
   W05 scene/entity identity and keep `AssetHandle` lease ownership intact.

## 8. Status Recommendation

`ready_for_integration`
