#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace br {
enum class MusicStatus { Missing, Ready, Playing, Paused, Failed };

// Optional local soundtrack, decoded by the Windows multimedia driver already
// provided by the OS. It owns no game-audio mixer and installs no codec/library.
class BackgroundMusic {
public:
    BackgroundMusic();
    ~BackgroundMusic();
    BackgroundMusic(const BackgroundMusic&)=delete;
    BackgroundMusic& operator=(const BackgroundMusic&)=delete;

    // Loading does not play. If the default MP3 is absent, try a WAV or M4A
    // with the same filename stem. Missing music is a normal, silent state.
    bool load(const std::filesystem::path& path="assets/audio/music/numbers-temporex.mp3");
    // volume = master volume * music volume, in [0,1]. Menus and mute pause
    // playback without rewinding. Native mode/position polling is at most 4 Hz.
    void update(bool paused,bool muted,float volume,float dt);
    // Release the device and file immediately, including when the game exits.
    void stop();
    bool available() const;
    MusicStatus status() const;
    const std::string& error() const;
    // Cached diagnostics; these accessors never call the audio driver.
    std::uint32_t positionMilliseconds() const;
    std::uint32_t durationMilliseconds() const;
    std::uint32_t loopCount() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
