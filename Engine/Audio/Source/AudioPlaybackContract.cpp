#include "SHE/Audio/AudioPlaybackContract.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <utility>

namespace she
{
namespace
{
[[nodiscard]] std::string TrimCopy(const std::string_view value)
{
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0)
    {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
    {
        --end;
    }

    return std::string(value.substr(start, end - start));
}

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

[[nodiscard]] std::map<std::string, std::string> ParseFields(const std::string_view payload)
{
    std::map<std::string, std::string> fields;
    std::size_t cursor = 0;

    while (cursor < payload.size())
    {
        const std::size_t separator = payload.find(';', cursor);
        const std::string_view token = payload.substr(
            cursor,
            separator == std::string_view::npos ? std::string_view::npos : separator - cursor);
        const std::size_t assignment = token.find('=');
        if (assignment != std::string_view::npos)
        {
            const std::string key = ToLowerCopy(TrimCopy(token.substr(0, assignment)));
            const std::string value = TrimCopy(token.substr(assignment + 1));
            if (!key.empty())
            {
                fields.insert_or_assign(key, value);
            }
        }

        if (separator == std::string_view::npos)
        {
            break;
        }

        cursor = separator + 1;
    }

    return fields;
}

[[nodiscard]] bool ParseBool(const std::string_view value)
{
    const std::string normalized = ToLowerCopy(value);
    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
}
} // namespace

std::string_view GetAudioPlaybackKindName(const AudioPlaybackKind kind)
{
    switch (kind)
    {
    case AudioPlaybackKind::Music:
        return "music";

    case AudioPlaybackKind::Sound:
    default:
        return "sound";
    }
}

std::string BuildGameplayAudioPlayPayload(const AudioPlaybackRequest& request)
{
    std::ostringstream stream;
    stream << "asset=" << request.assetName;
    stream << "; kind=" << GetAudioPlaybackKindName(request.kind);

    if (!request.channelName.empty())
    {
        stream << "; channel=" << request.channelName;
    }

    if (!request.groupName.empty())
    {
        stream << "; group=" << request.groupName;
    }

    stream << "; loop=" << (request.looping ? "true" : "false");
    stream << "; volume=" << std::fixed << std::setprecision(2) << request.volume;
    return stream.str();
}

std::optional<AudioPlaybackRequest> ParseGameplayAudioPlayPayload(const std::string_view payload)
{
    const auto fields = ParseFields(payload);
    const auto asset = fields.find("asset");
    if (asset == fields.end() || asset->second.empty())
    {
        return std::nullopt;
    }

    AudioPlaybackRequest request;
    request.assetName = asset->second;

    const auto kind = fields.find("kind");
    if (kind != fields.end() && ToLowerCopy(kind->second) == "music")
    {
        request.kind = AudioPlaybackKind::Music;
    }

    const auto channel = fields.find("channel");
    if (channel != fields.end())
    {
        request.channelName = channel->second;
    }

    const auto group = fields.find("group");
    if (group != fields.end())
    {
        request.groupName = group->second;
    }

    const auto looping = fields.find("loop");
    if (looping != fields.end())
    {
        request.looping = ParseBool(looping->second);
    }

    const auto volume = fields.find("volume");
    if (volume != fields.end())
    {
        try
        {
            request.volume = std::stof(volume->second);
        }
        catch (...)
        {
            request.volume = 1.0F;
        }
    }

    return request;
}

std::string BuildGameplayAudioStopPayload(const AudioStopRequest& request)
{
    std::ostringstream stream;
    bool wroteField = false;

    if (!request.channelName.empty())
    {
        stream << "channel=" << request.channelName;
        wroteField = true;
    }

    if (!request.groupName.empty())
    {
        if (wroteField)
        {
            stream << "; ";
        }

        stream << "group=" << request.groupName;
        wroteField = true;
    }

    if (!request.assetName.empty())
    {
        if (wroteField)
        {
            stream << "; ";
        }

        stream << "asset=" << request.assetName;
    }

    return stream.str();
}

std::optional<AudioStopRequest> ParseGameplayAudioStopPayload(const std::string_view payload)
{
    const auto fields = ParseFields(payload);

    AudioStopRequest request;
    if (const auto channel = fields.find("channel"); channel != fields.end())
    {
        request.channelName = channel->second;
    }

    if (const auto group = fields.find("group"); group != fields.end())
    {
        request.groupName = group->second;
    }

    if (const auto asset = fields.find("asset"); asset != fields.end())
    {
        request.assetName = asset->second;
    }

    if (request.channelName.empty() && request.groupName.empty() && request.assetName.empty())
    {
        return std::nullopt;
    }

    return request;
}
} // namespace she
