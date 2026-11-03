#include "game_internal.h"

namespace br::app {

bool Game::updateGameplay() {
    // Preserve the frame order: doors -> player -> streaming -> interactions -> actors.
    // Shifting uses the same pre-movement threat position as door collision.
    const auto threatPosition=threat.active() ? std::optional<WorldPoint>{{threat.x(),threat.z()}} : std::nullopt;
    doorVisualDirty |= world.advanceDoors(dt,player.x,player.z,.23,input.paused,threatPosition,Threat::RADIUS);
    updatePlayerMovement();
    timing.moveMs = (glfwGetTime() - timing.moveStart) * 1000;
    updateWorldStreaming(threatPosition);
    if(!updateLevelExit())return false;

    const InteractionResult interactions=updateInteractions();
    updateCreatures(interactions);
    updateEncounter();
    updateThreatAnimation();
    return true;
}

void Game::updatePlayerMovement() {
    // Move a circular player collider in small, axis-separated steps. This
    // permits sliding along walls while preventing tunnelling through them.
    applyMovementScenario();
    forward = input.forward();
    moving = false;
    frameDistance = 0;
    if (!input.paused) {
        player.crouched = player.hiding || world.headClearance(player.x,player.z,.23)<1.8f || input.down(GLFW_KEY_LEFT_CONTROL) || input.down(GLFW_KEY_RIGHT_CONTROL) ||
            scenarioCrouchHeld();
        const float targetHeight = player.crouched ? 1.08f : 1.65f;
        player.eyeHeight += (targetHeight - player.eyeHeight) * (1.f - std::exp(-dt * 12.f));
    }
    sprinting = !input.paused && !player.crouched && player.stamina.canSprint() && (input.down(GLFW_KEY_LEFT_SHIFT) ||
        scenarioSprintHeld());
    if (!input.paused && !options.tour && !player.hiding) {
        float mx = (input.down(GLFW_KEY_D) ? 1.f : 0.f) -
                   (input.down(GLFW_KEY_A) ? 1.f : 0.f);
        float mz = forwardMovementInput();
        const float length = std::sqrt(mx * mx + mz * mz);
        Vec3 desired{};
        if (length > 0) {
            const Vec3 front = normalize({forward.x, 0, forward.z});
            const Vec3 right = normalize(cross(front, {0, 1, 0}));
            const float speed = player.crouched ? CROUCH_SPEED : sprinting ? SPRINT_SPEED : WALK_SPEED;
            desired = (right * (mx / length) + front * (mz / length)) * speed;
        }
        const Vec3 delta = player.motion.advance(desired, dt);
        const int steps = std::max(1, int(std::ceil(std::hypot(delta.x, delta.z) / .08f)));
        const double oldX = player.x, oldZ = player.z;
        for (int i = 0; i < steps; ++i) {
            const double x = player.x + delta.x / steps;
            const double z = player.z + delta.z / steps;
            if (!world.blocked(x, player.z, .23) && world.headClearance(x,player.z,.23)>player.eyeHeight+.12f) player.x = x;
            else player.motion.velocity.x = 0;
            if (!world.blocked(player.x, z, .23) && world.headClearance(player.x,z,.23)>player.eyeHeight+.12f) player.z = z;
            else player.motion.velocity.z = 0;
        }
        frameDistance = std::hypot(player.x - oldX, player.z - oldZ);
        player.travelDistance += frameDistance;
        player.walkDistance += frameDistance;
        moving = frameDistance > .0001;
    } else {
        player.motion.reset();
    }

    player.stamina.update(dt, sprinting && moving, input.paused,world.infiniteStamina());
    player.minimumStamina = std::min(player.minimumStamina, player.stamina.value);
}

void Game::updateWorldStreaming(const std::optional<WorldPoint>& threatPosition) {
    const double worldStart = glfwGetTime();
    // Keep CPU and GPU working sets bounded as the player crosses chunks.
    world.update(player.x, player.z);
    world.prepareAhead(player.x, player.z); // One CPU mesh per frame, nearest first.
    maxChunks = std::max(maxChunks, world.chunks().size());
    maxPrepared = std::max(maxPrepared, world.preparedChunks().size());
    if (!input.paused && shifting) {
        shiftClock += dt;
        if (shiftClock > 14) {
            std::vector<WorldPoint> protectedParty;
            for(const auto& a:partygoers.actors)if(a.active)protectedParty.push_back({a.x,a.z});
            world.shiftBehind(player.x, player.z, forward.x, forward.z,threatPosition,protectedParty);
            shiftClock = 0;
        }
    }
    timing.worldMs = (glfwGetTime() - worldStart) * 1000;
}

bool Game::updateLevelExit() {
    // Rearm only after returning to the front of the threshold, not when
    // closing the door or walking deeper into the exit vestibule.
    if(player.z>world.exitLocation().z-.95) exitArmed=true;
    if(!input.paused && exitArmed && world.exitCrossed(player.x,player.z)) {
        log("Exit reached by walking through the open doorway.");
        if(world.level()==Level::Zero) enterLevel(Level::Fun);
        else if(world.isFun()) enterLevel(Level::Parking);
        else {
            flow.escaped=true;exitUsed=true;exitArmed=false;player.motion.reset();setScreen(Screen::Menu);menuSelection=0;
            notice.clear();log("Parking completed. All three circuits restored; service exit reached.");
            startEnding("parking exit");
            return false;
        }
    }
    return true;
}

void Game::updateCreatures(const InteractionResult& interactions) {
    // Actual movement and explicit interaction noise feed one physical actor.
    // Menu time never advances its senses, path following, or grace period.
    const auto previousThreatState=threat.state();
    applyAraneaContactScenario();
    ThreatPlayer perceivedPlayer{player.x,player.z,forward.x,forward.z,dt>0 ? float(frameDistance/dt) : 0.f,player.crouched,interactions.doorNoise,player.hiding};
    if(interactions.pickedUpKey && !options.noThreat && !options.threatPreview)
        threat.hearAlarm(world,perceivedPlayer,player.x,player.z,true);
    applyMouseContactScenario(perceivedPlayer);
    updateMouse(perceivedPlayer);
    applyMousePreviewScenario();
    applyPartygoerScenario();
    updatePartygoers();
    applyDarknessScenario();
    updateDarkness();
    updateAranea(perceivedPlayer,previousThreatState);
}

void Game::updateMouse(const ThreatPlayer& perceivedPlayer) {
    if(world.isFun() && !input.paused && encounter.alive() && presentMouseTime>0) {
        presentMouseTime=std::max(0.f,presentMouseTime-dt);
        if(presentMouseTime==0)mouse.active=false;
    }
    if(mouse.update(world,player.x,player.z,dt,input.paused || !encounter.alive() || (world.isFun() && !mouse.active),player.hiding,world.level()==Level::Zero)) {
        audio.play(SoundKind::MouseSqueak,{mouse.x,.12,mouse.z},1.f);
        if(!options.noThreat) threat.hearAlarm(world,perceivedPlayer,mouse.x,mouse.z);
        ++mouseSqueaks;log("Mouse squeaked / creature alerted.");
    }
    if(mouse.ambientCue && std::hypot(mouse.x-player.x,mouse.z-player.z)<16) {
        const float transmission=world.soundTransmission(player.x,player.z,mouse.x,mouse.z);
        audio.play(SoundKind::MouseAmbient,{mouse.x,.12,mouse.z},.7f*transmission);
    }
}

void Game::updatePartygoers() {
    int gazeWidth=0,gazeHeight=0;glfwGetFramebufferSize(window,&gazeWidth,&gazeHeight);
    partygoers.update(world,player.x,player.z,player.eyeHeight+player.bobOffset,forward,fieldOfView,
        float(std::max(1,gazeWidth))/std::max(1,gazeHeight),dt,
        input.paused || !encounter.alive() || options.noThreat);
    if(!input.paused && !options.noThreat && encounter.alive() && partygoers.touchingPlayer(world,player.x,player.z) &&
       (encounter.catchPlayer() || encounter.defeatImmediately())) {
        ++catches;player.motion.reset();moving=false;frameDistance=0;
        setScreen(Screen::Menu);menuSelection=0;notice.clear();noticeTime=0;
        audio.reset(static_cast<std::uint32_t>(world.seed()));music.update(true,true,0,0);
        scareYaw=scareTargetYaw=input.yaw;scarePitch=scareTargetPitch=input.pitch;scareApproach=forward;
        if(scareConfig.audio && encounter.presenting()) {
            audio.play(SoundKind::PartygoerJumpscare,{player.x,player.eyeHeight,player.z},1.f);
            log("Partygoer sound: supplied jumpscare.");
        }
        log(encounter.presenting()?"Partygoer contact: jumpscare started.":"Partygoer contact: game over. Restart at Level Fun checkpoint.");
    }
}

void Game::updateDarkness() {
    const int beforeDispel=darkness.dispelled,beforeSpawn=darkness.spawned;
    const bool darknessContact=darkness.update(world,player.x,player.z,player.eyeHeight+player.bobOffset,forward,flashlight && !player.hiding,dt,
        input.paused || !encounter.alive() || options.noThreat);
    if(darkness.dispelled>beforeDispel) {
        audio.play(SoundKind::ElectricalStinger,{player.x,player.eyeHeight,player.z},.16f);
        log("Darkness eyes dispersed by flashlight.");
    }
    if(darkness.spawned>beforeSpawn)for(const auto& eyes:darkness.eyes)if(eyes.active) {
        audio.play(SoundKind::AmbientCreak,{eyes.x,eyes.y,eyes.z},.35f);break;
    }
    if(darknessContact && (encounter.catchPlayer() || encounter.defeatImmediately())) {
        scareYaw=scareTargetYaw=input.yaw;scarePitch=scareTargetPitch=input.pitch;scareApproach=forward;
        ++catches;player.motion.reset();moving=false;frameDistance=0;
        setScreen(Screen::Menu);menuSelection=0;notice.clear();noticeTime=0;
        audio.reset(static_cast<std::uint32_t>(world.seed()));music.update(true,true,0,0);
        if(scareConfig.audio && encounter.presenting()) {
            audio.play(SoundKind::DarknessJumpscare,{player.x,player.eyeHeight,player.z},1.f);
            log("Darkness sound: electrical shriek.");
        }
        log(encounter.presenting()?"Darkness contact: jumpscare started.":"Darkness contact: game over (jumpscare disabled).");
    }
}

void Game::updateAranea(const ThreatPlayer& perceivedPlayer,ThreatState previousThreatState) {
    oldThreatX=threat.x();oldThreatZ=threat.z();
    // Key ownership keeps the speed bonus; only pickup triggers an alarm.
    threat.setEnraged(world.keyCollected());
    if(!options.noThreat && !options.threatPreview && world.level()==Level::Zero)
        threat.update(world,perceivedPlayer,dt,input.paused || !encounter.alive());
    if(threat.state()!=previousThreatState) {
        if(previousThreatState==ThreatState::Dormant) {
            threatBodyYaw=threat.yaw();threatFootPlanting.reset();
            threatGaitPhase=threatVisualSpeed=threatTurnWeight=0;lastThreatContact={};
        }
        std::ostringstream threatEvent;
        threatEvent << "Threat: " << threatStateName(threat.state()) << " / distance "
                    << std::fixed << std::setprecision(2) << std::hypot(threat.x()-player.x,threat.z()-player.z) << " m";
        log(threatEvent.str());
    }
    // Visual acquisition also fires when a sound already started the chase;
    // a held sight line repeats it. Hearing alone never cues a notice.
    const bool threatCanVocalize=world.level()==Level::Zero && threat.active()&&!threat.caught()&&!options.noThreat;
    for(const auto cue:threatVocals.update(dt,threatCanVocalize,
        threatCanVocalize&&threat.canSeePlayer()&&!player.hiding,false,
        input.paused||options.threatPreview,!options.noThreat&&encounter.alive()&&world.level()==Level::Zero)) {
        audio.play(cue,{threat.x(),1.8,threat.z()},1.f);
        if(cue==SoundKind::ThreatNotice)++threatNotices;else ++threatRoars;
        log(cue==SoundKind::ThreatNotice?"Threat audio cue: visual notice.":"Threat audio cue: rage roar.");
    }
}

void Game::updateEncounter() {
    encounter.update(dt,scareSuspended);
    const bool debugScare=world.level()==Level::Zero && input.hit(GLFW_KEY_J) && encounter.alive() && !flow.is(Screen::Settings);
    if(debugScare && !threat.active()) {
        threat.resetAt(player.x+forward.x*2,player.z+forward.z*2);
    }
    const float scareDistance=float(std::hypot(threat.x()-player.x,threat.z()-player.z));
    const bool nearScare=!player.hiding && !input.paused && !options.noThreat && threat.active() &&
        encounter.shouldTrigger(scareDistance,
            scareDistance<=scareConfig.triggerDistance && Threat::visibleSegment(world,player.x,player.z,threat.x(),threat.z()),threat.caught());
    if((debugScare || nearScare) && encounter.catchPlayer()) {
        scareYaw=input.yaw; scarePitch=input.pitch;
        const float cy=std::cos(threatBodyYaw),sy=std::sin(threatBodyYaw);
        const Vec3 toHead{float(threat.x()-player.x)+threatHeadLocal.x*cy-threatHeadLocal.z*sy,
            threatHeadLocal.y-player.eyeHeight-player.bobOffset,float(threat.z()-player.z)+threatHeadLocal.x*sy+threatHeadLocal.z*cy};
        scareApproach=dot(toHead,toHead)>.01f?normalize(toHead):forward;
        scareTargetYaw=std::atan2(scareApproach.z,scareApproach.x)*180/PI;
        scareTargetPitch=std::asin(std::clamp(scareApproach.y,-1.f,1.f))*180/PI;
        ++catches; player.motion.reset(); moving=false; frameDistance=0;threatFootPlanting.reset();
        setScreen(Screen::Menu); menuSelection=0;
        notice.clear(); noticeTime=0;
        music.update(true,true,0,0);
        audio.reset(static_cast<std::uint32_t>(world.seed()+catches));
        if(scareConfig.audio) audio.play(SoundKind::Jumpscare,{player.x,player.eyeHeight,player.z},1.f);
        log("Captured: 3D jumpscare started.");
    }
}

void Game::updateThreatAnimation() {
    if((!input.paused || encounter.scaring()) && !scareSuspended && !options.threatPreview) {
        threatAnimation+=dt;
        if(encounter.alive()) {
            const double travelX=threat.x()-oldThreatX,travelZ=threat.z()-oldThreatZ;
            // The AI turns before translation; rendering must not add a
            // second rotation lag that makes the running body crabwalk.
            const float previousBodyYaw=threatBodyYaw;
            threatBodyYaw=std::hypot(travelX,travelZ)>.00001?
                float(std::atan2(travelX,-travelZ)):threat.yaw();
            const float turnAngle=std::abs(std::remainder(threatBodyYaw-previousBodyYaw,2*PI));
            const float turnRate=dt>0?turnAngle/dt:0;
            const bool pivoting=threat.speed()<.15f && turnRate>.08f;
            const float poseSpeed=std::max(threat.speed(),pivoting?std::min(1.35f,turnRate*.6f):0.f);
            threatVisualSpeed+=(poseSpeed-threatVisualSpeed)*(1.f-std::exp(-dt*9.f));
            threatTurnWeight+=((pivoting?1.f:0.f)-threatTurnWeight)*(1.f-std::exp(-dt*16.f));
            // Translation retains the existing distance-driven cycle.
            // Pivoting now lifts/replants the feet instead of spinning a
            // frozen idle pose; a half turn advances one full walk cycle.
            const float stride=threatStrideLength(threatVisualSpeed);
            threatGaitPhase=std::fmod(threatGaitPhase+threat.speed()*dt/stride+
                (pivoting?turnAngle/PI:0.f),1.f);
            const auto contacts=threatFootContacts(threatVisualSpeed,threatGaitPhase);
            // Match the actual blended ground gait, including fast Search;
            // hysteresis prevents the sound bank chattering at the boundary.
            if(threatVisualSpeed>2.2f)threatRunningSteps=true;
            else if(threatVisualSpeed<1.9f)threatRunningSteps=false;
            if(threat.active() && (threat.speed()>.15f || pivoting)) {
                int landings=0;
                for(std::size_t foot=0;foot<contacts.feet.size();++foot)
                    landings+=contacts.feet[foot]>.5f && lastThreatContact.feet[foot]<=.5f;
                for(int landing=0;landing<landings;++landing) {
                    audio.play(threatRunningSteps?SoundKind::ThreatRunStep:SoundKind::ThreatStep,
                               {threat.x(),.2,threat.z()},.8f);
                    ++threatFootfalls;
                    if(threatRunningSteps)++threatRunFootfalls;else ++threatWalkFootfalls;
                }
            }
            lastThreatContact=contacts;
            float headTarget=0,pitchTarget=0;
            if(threat.speed()>.15f) {
                // Keep the corrected skull forward during locomotion.
                // Small baked nods remain; tracking turns happen at rest.
                headTarget=0;
                if(threat.canSeePlayer())pitchTarget=std::atan2(player.eyeHeight-THREAT_EYE_HEIGHT,float(std::max(1.,std::hypot(player.x-threat.x(),player.z-threat.z()))));
            } else if(threat.canSeePlayer()) {
                headTarget=std::remainder(float(std::atan2(player.x-threat.x(),-(player.z-threat.z())))-threatBodyYaw,2*PI);
                pitchTarget=std::atan2(player.eyeHeight-THREAT_EYE_HEIGHT,float(std::max(1.,std::hypot(player.x-threat.x(),player.z-threat.z()))));
            } else headTarget=std::sin(threatAnimation*.65f)*.32f;
            headTarget=std::clamp(headTarget,-.8f,.8f);
            pitchTarget=std::clamp(pitchTarget,-.25f,.20f);
            if(threat.speed()>.15f)threatHeadYaw=0;
            else threatHeadYaw+=std::clamp((headTarget-threatHeadYaw)*(1-std::exp(-dt*8.f)),-dt*2.5f,dt*2.5f);
            threatHeadPitch+=(pitchTarget-threatHeadPitch)*(1-std::exp(-dt*7.f));
        }
    }
}

} // namespace br::app
