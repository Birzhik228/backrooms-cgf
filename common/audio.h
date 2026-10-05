#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace br {
inline constexpr int AUDIO_SAMPLE_RATE=22050;
enum class Surface { Carpet, HardFloor, DampCarpet };
enum class Gait { Walk, Crouch, Sprint };
enum class SoundKind { Footstep, ClothRustle, AmbientCreak, WaterDrip, ElectricalStinger, Door };
struct SoundEvent {
    SoundKind kind=SoundKind::Footstep;
    Gait gait=Gait::Walk;
    Surface surface=Surface::Carpet;
    std::uint32_t variation=1;
};

// Load the six recorded footfalls once, before gameplay; no per-step disk reads.
// A missing or invalid bank returns false and keeps the procedural fallback.
bool loadFootstepSamples(const std::string& directory="assets/audio/footsteps");
std::size_t recordedFootstepCount();
// Strict bounded PCM WAV reader, also used by the CPU validation tests.
std::vector<std::int16_t> readFootstepWav(const std::string& path);
// Footsteps use the cached recordings; cloth and the missing-bank fallback are
// synthesized. Tests and the offline preview never open an audio device.
std::vector<std::int16_t> synthesizeSound(const SoundEvent& event);
class FootstepCadence {
public:
    void reset(bool crouched=false);
    std::vector<SoundEvent> update(float dt,float actualDistance,bool crouched,
                                  bool sprinting,Surface surface,bool paused=false);
private:
    double distance_=0;
    bool crouched_=false,walking_=false;
    Gait previousGait_=Gait::Walk;
    std::uint32_t variation_=1;
};
bool writeAudioPreview(const std::string& path);

// Double-precision WORLD positions: audio must not jump when graphics rebase.
struct AudioPosition { double x=0,y=0,z=0; };
struct AudioScene {
    AudioPosition listener{},forward{0,0,-1},fluorescent{3,3.2,3};
    float lightPower=1,lightInstability=0,tension=0;
};
struct StereoGain { float left=0,right=0; };
StereoGain positionalGains(const AudioScene& scene,AudioPosition source,
                           float nearDistance=2,float farDistance=48);

// The same bounded, cached stereo mixer drives waveOut and offline tests.
// New ambience is synthesized once at construction, never in the render loop.
class AudioMixer {
public:
    AudioMixer();
    ~AudioMixer();
    AudioMixer(const AudioMixer&)=delete;
    AudioMixer& operator=(const AudioMixer&)=delete;
    void setScene(const AudioScene& scene);
    void setVolume(float volume);
    void reset(std::uint32_t seed=1);
    void setPaused(bool paused);
    void enqueue(const SoundEvent& event);
    void play(SoundKind kind,AudioPosition position,float gain=1);
    void render(std::int16_t* interleavedStereo,std::size_t frames);
    std::size_t activeVoices() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Polled waveOut buffers: audio has no callbacks or worker threads.
// Start/stop implement the M-key mute, and unavailable audio is harmless.
class Audio {
public:
    Audio();
    ~Audio();
    Audio(const Audio&)=delete;
    Audio& operator=(const Audio&)=delete;
    void start();
    void stop();
    void setScene(const AudioScene& scene);
    void setVolume(float volume);
    void reset(std::uint32_t seed=1);
    void play(SoundKind kind,AudioPosition position,float gain=1);
    void update(float dt,float actualDistance,bool crouched,bool sprinting,
                Surface surface=Surface::Carpet,bool paused=false);
    bool available() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    FootstepCadence cadence_;
};
}
