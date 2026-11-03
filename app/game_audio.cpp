#include "game_internal.h"

namespace br::app {
namespace fs = std::filesystem;

void Game::updateAudio() {
    const double audioStart = glfwGetTime();
    currentRoom = roomInfo(int64_t(std::floor(player.x / CELL)), int64_t(std::floor(player.z / CELL)), world.seed(),world.level());
    const auto floorSurface = world.isParking()?FloorSurface::Tile:world.isFun()?FloorSurface::Carpet:floorSurfaceAt(player.x, player.z, world.seed());
    const Surface surface = floorSurface==FloorSurface::Tile ? Surface::HardFloor :
        floorSurface==FloorSurface::DampCarpet ? Surface::DampCarpet : Surface::Carpet;
    AudioScene soundScene;
    soundScene.listener={player.x,player.eyeHeight,player.z};
    soundScene.forward={forward.x,forward.y,forward.z};
    double nearestSoundLight=1e30;
    float localIllumination=0, localInstability=0;
    for(const auto& [coord,chunk] : world.chunks()) for(const auto& lamp : chunk.lamps) {
        const double lx=coord.x*CHUNK_SIZE+lamp.x, lz=coord.z*CHUNK_SIZE+lamp.z;
        const double distance=(lx-player.x)*(lx-player.x)+(lz-player.z)*(lz-player.z)+(lamp.y-player.eyeHeight)*(lamp.y-player.eyeHeight);
        if(distance<100) {
            const float envelope=lampEnvelope(lamp.state,lamp.phase,float(elapsed));
            localIllumination+=lamp.power*envelope/float(1+distance*.22);
            if(lamp.state==LampState::Dying) localInstability=std::max(localInstability,1.f/float(1+distance*.06));
            if(distance<nearestSoundLight && lamp.state!=LampState::Off) {
                nearestSoundLight=distance; soundScene.fluorescent={lx,lamp.y,lz};
                soundScene.lightPower=lamp.power*envelope;
                soundScene.lightInstability=lamp.state==LampState::Dying ? 1.f : 0.f;
            }
        }
    }
    if(nearestSoundLight==1e30) soundScene.lightPower=0;
    const float environmentalDanger=std::clamp((1.f-std::min(1.f,localIllumination))*.58f +
        localInstability*.28f + (flashlight ? 0.f : .15f) + (currentRoom.kind==RoomKind::OddRoom ? .22f : 0.f),0.f,1.f);
    const float danger=world.isFun()?0.f:std::max(environmentalDanger,threat.state()==ThreatState::Chase ? 1.f :
        threat.state()==ThreatState::Search ? .7f : 0.f);
    if(!input.paused) tension+=(danger-tension)*(1.f-std::exp(-dt*.6f));
    occlusionClock-=dt;
    if(occlusionClock<=0) {
        threatTransmission=threat.active()?world.soundTransmission(player.x,player.z,threat.x(),threat.z()):1;
        occlusionClock=.1f;
    }
    soundScene.threatTransmission=threatTransmission;
    soundScene.level0Atmosphere=world.level()==Level::Zero;
    soundScene.tension=encounter.alive()?tension:0;
    if(!encounter.alive()) soundScene.lightPower=0;
    AtmosphereContext atmosphereState;
    atmosphereState.listener=soundScene.listener;
    atmosphereState.paused=input.paused||options.threatPreview;
    atmosphereState.alive=encounter.alive();
    atmosphereState.hidden=player.hiding;
    atmosphereState.threatActive=world.level()==Level::Zero&&threat.active()&&!options.noThreat;
    atmosphereState.threat={threat.x(),1.35,threat.z()};
    atmosphereState.threatDanger=threat.state()==ThreatState::Chase||threat.state()==ThreatState::Search;
    const double hintDistance=std::hypot(threat.x()-player.x,threat.z()-player.z);
    // Visibility here is the player's view, not the creature's sight cone.
    atmosphereState.threatVisible=atmosphereState.threatActive&&threatTransmission>.99f&&
        ((threat.x()-player.x)*forward.x+(threat.z()-player.z)*forward.z)>.15*hintDistance;
    PresenceContext presenceState;
    presenceState.listener={player.x,player.z};presenceState.threat={threat.x(),threat.z()};
    presenceState.forwardX=forward.x;presenceState.forwardZ=forward.z;
    presenceState.paused=input.paused||(options.threatPreview&&!options.presencePreview);
    presenceState.alive=encounter.alive();presenceState.hidden=player.hiding;
    presenceState.threatActive=atmosphereState.threatActive;
    presenceState.threatWandering=threat.state()==ThreatState::Wander && std::abs(input.pitch)<40;
    const auto previousRattle=presence.doorId();
    const float previousHandleAngle=presence.handleAngle();
    std::optional<DoorRattleCue> rattle;
    // Only the developer preview advances a real, fixed creature/door
    // fixture to its first event. Ordinary play always uses simulation dt.
    if(options.presencePreview && frame==0) {
        for(int attempt=0;attempt<240&&!rattle;++attempt)rattle=presence.update(world,presenceState,.1f);
        if(rattle)presence.update(world,presenceState,.1f);
    } else rattle=presence.update(world,presenceState,dt);
    if(previousRattle!=presence.doorId() || previousHandleAngle!=presence.handleAngle())doorVisualDirty=true;
    if(rattle) {
        audio.play(SoundKind::Door,{rattle->position.x,1.1,rattle->position.z},.35f);
        ++presenceEvents;
        log("Level 0 presence: door handle / "+std::to_string(rattle->doorId));
    }
    // Keep the short mechanical cue clear of unrelated ambient one-shots.
    if(presence.doorId())atmosphereState.paused=true;
    soundScene.atmosphereQuiet=!atmosphereState.alive||atmosphereState.paused||atmosphereState.hidden||
        atmosphereState.threatDanger||atmosphereState.threatVisible||
        (atmosphereState.threatActive&&hintDistance<10);
    FunAudioContext funAudioState;
    funAudioState.listener=soundScene.listener;
    funAudioState.paused=input.paused;
    funAudioState.alive=encounter.alive();
    auto funCues=funAudio.update(world,partygoers,funAudioState,dt);
    // Developer capture advances only the decorative schedule, using real
    // seeded props and with actors disabled. Ordinary play uses dt above.
    if(options.funAmbiencePreview && options.noThreat && world.isFun() && frame==3 &&
       !input.paused && encounter.alive()) {
        for(int attempt=0;attempt<900 && funCues.size()<2;++attempt) {
            const auto preview=funAudio.update(world,partygoers,funAudioState,.1f);
            funCues.insert(funCues.end(),preview.begin(),preview.end());
        }
    }
    soundScene.funAtmosphere=world.isFun();
    soundScene.partyMovingMask=funAudio.movingMask();
    if(world.isFun())soundScene.atmosphereQuiet=funAudio.quiet();
    ParkingAudioContext parkingAudioState;
    parkingAudioState.listener=soundScene.listener;
    parkingAudioState.paused=input.paused;parkingAudioState.alive=encounter.alive();
    auto parkingCues=parkingAudio.update(world,darkness,parkingAudioState,dt);
    // Only the developer fixture advances the schedule; normal play uses dt.
    if(options.parkingAmbiencePreview && options.noThreat && world.isParking() &&
       frame==(options.autoBreakers?14:3) && !input.paused && encounter.alive()) {
        for(int attempt=0;attempt<1500 && parkingCues.size()<3;++attempt) {
            const auto preview=parkingAudio.update(world,darkness,parkingAudioState,.1f);
            parkingCues.insert(parkingCues.end(),preview.begin(),preview.end());
        }
    }
    soundScene.parkingAtmosphere=world.isParking();
    if(world.isParking())soundScene.atmosphereQuiet=parkingAudio.quiet();
    audio.setScene(soundScene);
    for(const auto& cue:parkingCues) {
        audio.play(cue.kind,cue.position,cue.gain,cue.transmission);
        if(cue.circuit>=0) {
            ++parkingRelayEvents;
            log("Parking power relay: "+std::string(1,char('A'+cue.circuit)));
        } else {
            if(cue.kind==SoundKind::PipeTick)++parkingPipeEvents;
            else if(cue.kind==SoundKind::WaterDrip)++parkingDripEvents;
            else ++parkingVentEvents;
            log(std::string("Parking ambience: ")+(cue.kind==SoundKind::PipeTick?"pipe tick":cue.kind==SoundKind::WaterDrip?"pipe drip":"ventilation"));
        }
    }
    for(const auto& cue:funCues) {
        if(cue.actor>=0) {
            audio.playPartyStep(cue.actor,cue.position,cue.gain,cue.transmission);
            if(std::hypot(cue.position.x-player.x,cue.position.z-player.z)<24)partyStepDuckTime=.5f;
            ++partyStepEvents;
            if(partygoers.actors[cue.actor].observed)++partyFrozenSteps;
        } else {
            audio.play(cue.kind,cue.position,cue.gain,cue.transmission);
            if(cue.kind==SoundKind::BalloonRub)++funBalloonEvents;else ++funMelodyEvents;
            log(std::string("Level Fun ambience: ")+(cue.kind==SoundKind::BalloonRub?"balloon rub":"toy melody"));
        }
    }
    if(const auto cue=atmosphere.update(world,atmosphereState,dt)) {
        audio.play(cue->kind,cue->position,cue->gain,cue->transmission);
        const bool steps=cue->kind==SoundKind::DistantFootsteps;
        const bool hint=cue->kind==SoundKind::ThreatRustle;
        if(steps)++atmosphereSteps;else if(hint)++atmosphereHints;else ++atmosphereAmbient;
        std::ostringstream event;
        event << "Level 0 atmosphere: " << (steps?"distant steps":hint?"Aranea rustle":"room ambience")
              << " / distance " << std::fixed << std::setprecision(2)
              << std::hypot(cue->position.x-player.x,cue->position.z-player.z)
              << " m / transmission " << cue->transmission;
        log(event.str());
    }
    audio.update(dt, float(frameDistance), player.crouched, sprinting && moving, surface, input.paused && (!encounter.presenting() || scareSuspended || !scareConfig.audio));
    if (!input.paused) {
        // Widen only for real sprint travel, not for Shift held at a wall.
        // Exponential easing has the same response at different frame rates.
        const float targetFov = settings.fov + (sprinting && moving ? 4.f : 0.f);
        fieldOfView += (targetFov - fieldOfView) * (1.f - std::exp(-10.f * dt));
    }
    if(!input.paused) {
        veritySongTime=std::max(0.f,veritySongTime-dt);
        partyStepDuckTime=std::max(0.f,partyStepDuckTime-dt);
    }
    music.update(input.paused || veritySongTime>0,muted || options.hidden || !settings.musicEnabled,
        settings.volume*settings.musicVolume*(partyStepDuckTime>0?.25f:1.f),dt);
    if(!musicErrorReported && !music.error().empty()) {
        log("Background music unavailable: "+music.error()); musicErrorReported=true;
    }
    timing.audioMs = (glfwGetTime() - audioStart) * 1000;
}

} // namespace br::app
