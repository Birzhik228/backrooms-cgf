#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace br {
enum class MusicStatus { Missing, Ready, Playing, Paused, Failed };

// Optional local soundtrack. Windows uses its multimedia driver; the SDL audio
// backend used on macOS loads WAV and owns a separate bounded PCM output queue.
class BackgroundMusic {
public:
    BackgroundMusic();
    ~BackgroundMusic();
    BackgroundMusic(const BackgroundMusic&)=delete;
    BackgroundMusic& operator=(const BackgroundMusic&)=delete;

    // Loading does not play. An absent path falls back to another supported
    // extension with the same stem. Missing music is a normal, silent state.
    bool load(const std::filesystem::path& path="assets/audio/music/threshold-ambient.wav");
    // volume = master volume * music volume, in [0,1]. Menus and mute pause
    // playback without rewinding. The SDL queue is serviced on each update;
    // Windows native mode/position polling is at most 4 Hz.
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
