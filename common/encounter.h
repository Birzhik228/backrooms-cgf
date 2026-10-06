#pragma once
#include <algorithm>
#include <cmath>

namespace br {
enum class EncounterPhase { Exploring, Jumpscare, GameOver };
class Encounter {
public:
    static constexpr float scareDuration=1.05f;
    bool catchPlayer() {
        if (phase_!=EncounterPhase::Exploring) return false;
        phase_=EncounterPhase::Jumpscare; elapsed_=0; return true;
    }
    void update(float dt,bool suspended=false) {
        if (phase_!=EncounterPhase::Jumpscare || suspended || !std::isfinite(dt) || dt<=0) return;
        elapsed_=std::min(scareDuration,elapsed_+dt);
        if (elapsed_>=scareDuration) phase_=EncounterPhase::GameOver;
    }
    void skipScare() { if(scaring()) { elapsed_=scareDuration; phase_=EncounterPhase::GameOver; } }
    void reset() { phase_=EncounterPhase::Exploring; elapsed_=0; }
    bool scaring() const { return phase_==EncounterPhase::Jumpscare; }
    bool gameOver() const { return phase_==EncounterPhase::GameOver; }
    bool alive() const { return phase_==EncounterPhase::Exploring; }
    float progress() const { return elapsed_/scareDuration; }
private:
    EncounterPhase phase_=EncounterPhase::Exploring;
    float elapsed_=0;
};
}
