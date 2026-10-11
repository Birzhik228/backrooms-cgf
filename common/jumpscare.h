#pragma once
#include <algorithm>
#include <cmath>

namespace br {
// Timings use simulation seconds; only the trigger flash counts rendered frames.
struct JumpscareConfig {
    float duration=2.f, lungeDuration=.22f, blackDelay=.30f;
    float triggerDistance=1.5f, faceDistance=.75f, closeupScale=2.4f;
    float shakeOffset=.026f, shakeRoll=.025f, fovStart=70.f, fovPeak=95.f;
    float chromaticAmount=.007f, staticAmount=.16f, vignetteAmount=.65f, redAmount=.6f;
    bool enabled=true, cameraTurn=true, lunge=true, shake=true, fovPunch=true;
    bool chromatic=true, noise=true, vignette=true, redTint=true, flash=true;
    bool glow=true, darken=true, audio=true;
};
inline constexpr JumpscareConfig JUMPSCARE{};
enum class JumpscareState { Inactive, Lunge, Hold, End };
class JumpscareManager {
public:
    explicit JumpscareManager(JumpscareConfig config=JUMPSCARE):config_(config) {
        if(!std::isfinite(config_.duration) || config_.duration<=0) config_.duration=2;
        if(!std::isfinite(config_.lungeDuration) || config_.lungeDuration<=0) config_.lungeDuration=.22f;
        config_.lungeDuration=std::min(config_.lungeDuration,config_.duration);
        if(!std::isfinite(config_.blackDelay) || config_.blackDelay<0) config_.blackDelay=.3f;
    }
    bool trigger() {
        if(!config_.enabled || state_!=JumpscareState::Inactive) return false;
        state_=JumpscareState::Lunge; elapsed_=0; flashFrames_=config_.flash?2:0; return true;
    }
    bool catchPlayer() { return trigger(); }
    void update(float dt,bool suspended=false) {
        if(alive() || suspended || !std::isfinite(dt) || dt<=0) return;
        elapsed_=std::min(config_.duration+config_.blackDelay,elapsed_+dt);
        state_=elapsed_>=config_.duration?JumpscareState::End:
               elapsed_>=config_.lungeDuration?JumpscareState::Hold:JumpscareState::Lunge;
    }
    bool shouldTrigger(float distance,bool lineOfSight,bool caught) const {
        return alive() && config_.enabled && (caught || (lineOfSight && std::isfinite(distance) && distance>=0 && distance<=config_.triggerDistance));
    }
    void skipScare() { if(!alive()) { elapsed_=config_.duration; state_=JumpscareState::End; flashFrames_=0; } }
    void reset() { state_=JumpscareState::Inactive; elapsed_=0; flashFrames_=0; }
    bool alive() const { return state_==JumpscareState::Inactive; }
    bool presenting() const { return state_==JumpscareState::Lunge || state_==JumpscareState::Hold; }
    bool scaring() const { return !alive() && !gameOver(); }
    bool black() const { return state_==JumpscareState::End; }
    bool gameOver() const { return black() && elapsed_>=config_.duration+config_.blackDelay; }
    float progress() const { return std::min(1.f,elapsed_/config_.duration); }
    float elapsed() const { return elapsed_; }
    float lunge() const { const float t=std::min(1.f,elapsed_/config_.lungeDuration); return 1-std::pow(1-t,3.f); }
    float flashForDraw() { if(!presenting() || !flashFrames_) return 0; --flashFrames_; return 1; }
    JumpscareState state() const { return state_; }
private:
    JumpscareConfig config_;
    JumpscareState state_=JumpscareState::Inactive;
    float elapsed_=0;
    int flashFrames_=0;
};
}
