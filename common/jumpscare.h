#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace br {
// Timings use simulation seconds; only the trigger flash counts rendered frames.
struct JumpscareConfig {
    float duration=2.f, cameraTurnDuration=.20f, lungeDuration=.22f, blackDelay=.30f;
    float triggerDistance=1.5f, faceDistance=.90f, closeupScale=2.2f;
    float shakeOffset=.026f, shakeRoll=.025f, fovStart=70.f, fovPeak=95.f;
    float chromaticAmount=.007f, staticAmount=.16f, vignetteAmount=.65f, redAmount=.6f;
    bool enabled=true, cameraTurn=true, lunge=true, shake=true, fovPunch=true;
    bool chromatic=true, noise=true, vignette=true, redTint=true, flash=true;
    bool glow=true, darken=true, audio=true;
};
inline constexpr JumpscareConfig JUMPSCARE{};

inline JumpscareConfig sanitizeJumpscareConfig(JumpscareConfig config) {
    const auto bounded=[](float value,float fallback,float low,float high) {
        return std::clamp(std::isfinite(value)?value:fallback,low,high);
    };
    config.duration=bounded(config.duration,JUMPSCARE.duration,.5f,6.f);
    config.cameraTurnDuration=bounded(config.cameraTurnDuration,JUMPSCARE.cameraTurnDuration,.15f,.25f);
    config.lungeDuration=bounded(config.lungeDuration,JUMPSCARE.lungeDuration,.10f,.50f);
    config.blackDelay=bounded(config.blackDelay,JUMPSCARE.blackDelay,0.f,2.f);
    config.triggerDistance=bounded(config.triggerDistance,JUMPSCARE.triggerDistance,.5f,3.f);
    config.faceDistance=bounded(config.faceDistance,JUMPSCARE.faceDistance,.6f,1.f);
    config.closeupScale=bounded(config.closeupScale,JUMPSCARE.closeupScale,1.f,6.f);
    config.shakeOffset=bounded(config.shakeOffset,JUMPSCARE.shakeOffset,0.f,.15f);
    config.shakeRoll=bounded(config.shakeRoll,JUMPSCARE.shakeRoll,0.f,.12f);
    config.fovStart=bounded(config.fovStart,JUMPSCARE.fovStart,50.f,110.f);
    config.fovPeak=bounded(config.fovPeak,JUMPSCARE.fovPeak,config.fovStart,120.f);
    config.chromaticAmount=bounded(config.chromaticAmount,JUMPSCARE.chromaticAmount,0.f,.03f);
    config.staticAmount=bounded(config.staticAmount,JUMPSCARE.staticAmount,0.f,.5f);
    config.vignetteAmount=bounded(config.vignetteAmount,JUMPSCARE.vignetteAmount,0.f,1.f);
    config.redAmount=bounded(config.redAmount,JUMPSCARE.redAmount,0.f,1.f);
    return config;
}

// Optional local settings: one key=value per line. Invalid entries cannot poison
// the camera matrices; unknown keys and incomplete lines retain safe defaults.
inline JumpscareConfig loadJumpscareConfig(const std::filesystem::path& path) {
    JumpscareConfig config;
    std::ifstream input(path);
    std::string line;
    const auto trim=[](const std::string& value) {
        const auto begin=value.find_first_not_of(" \t\r\n");
        if(begin==std::string::npos) return std::string{};
        return value.substr(begin,value.find_last_not_of(" \t\r\n")-begin+1);
    };
    while(std::getline(input,line)) {
        if(const auto comment=line.find('#');comment!=std::string::npos) line.resize(comment);
        const auto equals=line.find('=');
        if(equals==std::string::npos) continue;
        const std::string key=trim(line.substr(0,equals)),value=trim(line.substr(equals+1));
        float* field=nullptr;
        if(key=="duration") field=&config.duration;
        else if(key=="camera_turn_duration") field=&config.cameraTurnDuration;
        else if(key=="lunge_duration") field=&config.lungeDuration;
        else if(key=="black_delay") field=&config.blackDelay;
        else if(key=="trigger_distance") field=&config.triggerDistance;
        else if(key=="face_distance") field=&config.faceDistance;
        else if(key=="closeup_scale") field=&config.closeupScale;
        else if(key=="shake_offset") field=&config.shakeOffset;
        else if(key=="shake_roll") field=&config.shakeRoll;
        else if(key=="fov_start") field=&config.fovStart;
        else if(key=="fov_peak") field=&config.fovPeak;
        if(field) {
            std::istringstream number(value); float parsed=0; std::string trailing;
            if(number>>parsed && !(number>>trailing) && std::isfinite(parsed)) *field=parsed;
            continue;
        }
        bool* toggle=nullptr;
        if(key=="enabled") toggle=&config.enabled;
        else if(key=="camera_turn") toggle=&config.cameraTurn;
        else if(key=="lunge") toggle=&config.lunge;
        else if(key=="shake") toggle=&config.shake;
        else if(key=="fov_punch") toggle=&config.fovPunch;
        else if(key=="chromatic") toggle=&config.chromatic;
        else if(key=="noise") toggle=&config.noise;
        else if(key=="vignette") toggle=&config.vignette;
        else if(key=="red_tint") toggle=&config.redTint;
        else if(key=="flash") toggle=&config.flash;
        else if(key=="glow") toggle=&config.glow;
        else if(key=="darken") toggle=&config.darken;
        else if(key=="audio") toggle=&config.audio;
        if(toggle && (value=="true" || value=="1")) *toggle=true;
        else if(toggle && (value=="false" || value=="0")) *toggle=false;
    }
    return sanitizeJumpscareConfig(config);
}

struct JumpscareShake { float x=0,y=0,z=0,roll=0; };
enum class JumpscareState { Inactive, Lunge, Hold, End };
class JumpscareManager {
public:
    explicit JumpscareManager(JumpscareConfig config=JUMPSCARE):config_(sanitizeJumpscareConfig(config)) {}
    bool trigger() {
        if(!config_.enabled || state_!=JumpscareState::Inactive) return false;
        state_=JumpscareState::Lunge; elapsed_=0; flashFrames_=config_.flash?2:0; return true;
    }
    bool catchPlayer() { return trigger(); }
    // Contact death without a jumpscare, independent of scare configuration.
    bool defeatImmediately() {
        if(!alive())return false;
        state_=JumpscareState::End;elapsed_=config_.duration+config_.blackDelay;flashFrames_=0;
        return true;
    }
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
    const JumpscareConfig& config() const { return config_; }
    float cameraTurn() const { return config_.cameraTurn?easeOut(elapsed_/config_.cameraTurnDuration):0.f; }
    float lunge() const { return config_.lunge?easeOut(elapsed_/config_.lungeDuration):0.f; }
    float visualFov() const {
        if(!presenting() || !config_.fovPunch) return config_.fovStart;
        const float punchTime=std::min(.12f,config_.duration*.20f);
        if(elapsed_<=punchTime) return config_.fovStart+(config_.fovPeak-config_.fovStart)*easeOut(elapsed_/punchTime);
        const float hold=(elapsed_-punchTime)/(config_.duration-punchTime);
        const float envelope=std::pow(1.f-progress(),1.5f);
        return std::clamp(config_.fovPeak-6.f*hold*hold+1.6f*std::sin((elapsed_-punchTime)*29.f)*envelope,config_.fovStart,120.f);
    }
    JumpscareShake shake() const {
        if(!presenting() || !config_.shake) return {};
        const float envelope=std::pow(std::max(0.f,1.f-progress()),1.6f);
        const std::uint32_t sample=static_cast<std::uint32_t>(std::floor(elapsed_*90.f));
        const float offset=config_.shakeOffset*envelope,roll=config_.shakeRoll*envelope;
        return {randomSigned(sample,0x92d68ca2u)*offset,randomSigned(sample,0x4f1bbcdcu)*offset,
                randomSigned(sample,0xd1b54a35u)*offset,randomSigned(sample,0x94d049bbu)*roll};
    }
    float flashForDraw() { if(!presenting() || !flashFrames_) return 0; --flashFrames_; return 1; }
    JumpscareState state() const { return state_; }
private:
    static float easeOut(float t) { t=std::clamp(t,0.f,1.f); const float remaining=1.f-t; return 1.f-remaining*remaining*remaining; }
    static float randomSigned(std::uint32_t sample,std::uint32_t salt) {
        std::uint32_t hash=sample^salt;
        hash^=hash>>16; hash*=0x7feb352du; hash^=hash>>15; hash*=0x846ca68bu; hash^=hash>>16;
        return static_cast<float>(hash&0x00ffffffu)/8388607.5f-1.f;
    }
    JumpscareConfig config_;
    JumpscareState state_=JumpscareState::Inactive;
    float elapsed_=0;
    int flashFrames_=0;
};
}
