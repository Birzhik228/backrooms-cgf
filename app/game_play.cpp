#include "game_internal.h"

namespace br::app {
namespace fs = std::filesystem;

bool Game::updateGameplay() {
    // Advance leaves before movement so collision uses their visible angle.
    const auto threatPosition=threat.active() ? std::optional<WorldPoint>{{threat.x(),threat.z()}} : std::nullopt;
    doorVisualDirty |= world.advanceDoors(dt,px,pz,.23,input.paused,threatPosition,Threat::RADIUS);
    // Move a circular player collider in small, axis-separated steps. This
    // permits sliding along walls while preventing tunnelling through them.
    if(options.partyCheck && frame==60)input.setLook(90,0);
    if(options.partyCheck && frame==120)input.setLook(-90,0);
    forward = input.forward();
    moving = false;
    frameDistance = 0;
    if (!input.paused) {
        crouched = hiding || world.headClearance(px,pz,.23)<1.8f || input.down(GLFW_KEY_LEFT_CONTROL) || input.down(GLFW_KEY_RIGHT_CONTROL) ||
            (options.controlHoldCheck && frame>=2 && frame<32) ||
            (options.autoCrouch && !(options.autoStand && frame >= options.frames / 2));
        const float targetHeight = crouched ? 1.08f : 1.65f;
        eyeHeight += (targetHeight - eyeHeight) * (1.f - std::exp(-dt * 12.f));
    }
    sprinting = !input.paused && !crouched && stamina.canSprint() && (input.down(GLFW_KEY_LEFT_SHIFT) ||
        (options.autoSprint && !(options.autoReleaseSprint && frame >= options.frames / 2)));
    if (!input.paused && !options.tour && !hiding) {
        float mx = (input.down(GLFW_KEY_D) ? 1.f : 0.f) -
                   (input.down(GLFW_KEY_A) ? 1.f : 0.f);
        float mz = options.autoWalk ? ((options.autoStop && frame >= options.frames / 2) ? 0.f : 1.f) :
                   (input.down(GLFW_KEY_W) ? 1.f : 0.f) -
                   (input.down(GLFW_KEY_S) ? 1.f : 0.f);
        const float length = std::sqrt(mx * mx + mz * mz);
        Vec3 desired{};
        if (length > 0) {
            const Vec3 front = normalize({forward.x, 0, forward.z});
            const Vec3 right = normalize(cross(front, {0, 1, 0}));
            const float speed = crouched ? CROUCH_SPEED : sprinting ? SPRINT_SPEED : WALK_SPEED;
            desired = (right * (mx / length) + front * (mz / length)) * speed;
        }
        const Vec3 delta = motion.advance(desired, dt);
        const int steps = std::max(1, int(std::ceil(std::hypot(delta.x, delta.z) / .08f)));
        const double oldX = px, oldZ = pz;
        for (int i = 0; i < steps; ++i) {
            const double x = px + delta.x / steps;
            const double z = pz + delta.z / steps;
            if (!world.blocked(x, pz, .23) && world.headClearance(x,pz,.23)>eyeHeight+.12f) px = x;
            else motion.velocity.x = 0;
            if (!world.blocked(px, z, .23) && world.headClearance(px,z,.23)>eyeHeight+.12f) pz = z;
            else motion.velocity.z = 0;
        }
        frameDistance = std::hypot(px - oldX, pz - oldZ);
        travel += frameDistance;
        walk += frameDistance;
        moving = frameDistance > .0001;
    } else {
        motion.reset();
    }

    stamina.update(dt, sprinting && moving, input.paused,world.infiniteStamina());
    minimumStamina = std::min(minimumStamina, stamina.value);
    moveMs = (glfwGetTime() - moveStart) * 1000;
    const double worldStart = glfwGetTime();
    // Keep CPU and GPU working sets bounded as the player crosses chunks.
    world.update(px, pz);
    world.prepareAhead(px, pz); // One CPU mesh per frame, nearest first.
    maxChunks = std::max(maxChunks, world.chunks().size());
    maxPrepared = std::max(maxPrepared, world.preparedChunks().size());
    if (!input.paused && shifting) {
        shiftClock += dt;
        if (shiftClock > 14) {
            std::vector<WorldPoint> protectedParty;
            for(const auto& a:partygoers.actors)if(a.active)protectedParty.push_back({a.x,a.z});
            world.shiftBehind(px, pz, forward.x, forward.z,threatPosition,protectedParty);
            shiftClock = 0;
        }
    }
    worldMs = (glfwGetTime() - worldStart) * 1000;
    // Rearm only after returning to the front of the threshold, not when
    // closing the door or walking deeper into the exit vestibule.
    if(pz>world.exitLocation().z-.95) exitArmed=true;
    if(!input.paused && exitArmed && world.exitCrossed(px,pz)) {
        log("Exit reached by walking through the open doorway.");
        if(world.level()==Level::Zero) enterLevel(Level::Fun);
        else if(world.isFun()) enterLevel(Level::Parking);
        else {
            flow.escaped=true;exitUsed=true;exitArmed=false;motion.reset();setScreen(Screen::Menu);menuSelection=0;
            notice.clear();log("Parking completed. All three circuits restored; service exit reached.");
            startEnding("parking exit");
            return false;
        }
    }
    float doorNoise=0;
    if(options.hideFixture && frame==1) {
        const br::HidingSpot* selected=nullptr;
        for(const auto& entry:world.chunks()) for(const auto& spot:entry.second.hidingSpots)
            if(!selected || spot.id<selected->id) selected=&spot;
        if(selected) {
            px=selected->approach.x;pz=selected->approach.z;
            input.setLook(float(std::atan2(selected->inside.z-pz,selected->inside.x-px)*180/3.141592653589793),-12);
            log("Hiding fixture: "+std::to_string(px)+", "+std::to_string(pz));
        }
    }
    // Scripted fixtures use the same E interaction and present rolls as play.
    if(options.autoBreakers && world.isParking() && frame>=1 && frame<=9 && frame%4==1) {
        const auto panel=world.breakerLocation((frame-1)/4);
        px=panel.x;pz=panel.z+1;world.update(px,pz);motion.reset();input.setLook(-90,0);forward=input.forward();
        simulateKey(GLFW_KEY_E);
    }
    if(options.autoBreakers && options.autoExit && world.isParking() && frame==20) {
        const auto exit=world.exitLocation();px=exit.x;pz=exit.z;world.update(px,pz);motion.reset();input.setLook(-90,0);forward=input.forward();
    }
    if(options.autoBreakers && options.autoExit && world.isParking() && frame==21)simulateKey(GLFW_KEY_E);
    if(options.funPreview && options.autoKey && world.isFun() && !world.keyCollected() && frame>0 && frame<65 && frame%4==1) {
        double nearest=1e30;Present selected{};
        for(const auto& entry:world.chunks())for(const auto& gift:entry.second.presents)if(!gift.opened) {
            const double distance=std::hypot(gift.x-px,gift.z-pz);
            if(distance<nearest){nearest=distance;selected=gift;}
        }
        if(nearest<1e30){px=selected.x+1.05;pz=selected.z;input.setLook(180,-35);}
        world.update(px,pz);forward=input.forward();
        simulateKey(GLFW_KEY_E);
    }
    if(options.funPreview && options.autoKey && options.autoExit && world.keyCollected() && frame==80) {
        const auto exit=world.exitLocation();px=exit.x;pz=exit.z;world.update(px,pz);motion.reset();input.setLook(-90,0);
        forward=input.forward();
    }
    if(options.autoKey && frame==1 && world.level()==Level::Zero) {
        const auto key=world.keyLocation(); px=key.x;pz=key.z+1;world.update(px,pz);
    }
    if(options.autoKey && options.autoExit && frame==20 && world.level()==Level::Zero) {
        const auto exit=world.exitLocation();px=exit.x;pz=exit.z;world.update(px,pz);
    }
    const bool pickedUpKey=!input.paused && encounter.alive() && !hiding && world.collectKey(px,pz);
    if(pickedUpKey) {
        notice="KEY FOUND / EXIT UNLOCKED / IT HEARD YOU";noticeTime=5;
        log("Key collected. Exit unlocked; press E to open. Creature called once.");
    }
    nearbyPresent = {};
    nearbyBreaker=-1;
    nearBreaker=world.nearestBreaker(px,pz,1.8,nearbyBreaker);
    if(nearBreaker) {
        const auto p=world.breakerLocation(nearbyBreaker);
        nearBreaker=(p.x-px)*forward.x+(p.z-pz)*forward.z>.35*std::hypot(p.x-px,p.z-pz);
    }
    const bool breakerInteraction=nearBreaker && !input.paused && encounter.alive() && input.hit(GLFW_KEY_E);
    if(breakerInteraction && world.activateBreaker(nearbyBreaker,px,pz)) {
        notice="CIRCUIT "+std::string(1,char('A'+nearbyBreaker))+" ONLINE / SECTOR SAFE / "+std::to_string(world.breakersPowered())+" OF 3";
        if(world.exitUnlocked())notice+=" / SERVICE EXIT UNLOCKED";
        noticeTime=5;log(notice);nearBreaker=false;
    }
    nearPresent=world.nearestPresent(px,pz,1.8,nearbyPresent);
    if(nearPresent) {
        const double dx=nearbyPresent.x-px,dz=nearbyPresent.z-pz,d=std::hypot(dx,dz);
        nearPresent=facingPresent(nearbyPresent,px,pz,eyeHeight+bobOffset,forward);
        for(double along=.05;nearPresent && along<d-.46;along+=.05)
            if(world.blocked(px+dx*along/d,pz+dz*along/d,.05))nearPresent=false;
    }
    const bool presentInteraction=nearPresent && !input.paused && encounter.alive() && input.hit(GLFW_KEY_E);
    if(presentInteraction) {
        const auto result=world.openPresent(nearbyPresent.id,px,pz);
        if(result!=PresentResult::Invalid) {
            nearbyPresent.opened=true;
            audio.play(SoundKind::PresentOpen,{nearbyPresent.x,.4,nearbyPresent.z},1.f);
            nearPresent=false;
            notice=result==PresentResult::Key?"KEY FOUND / EXIT UNLOCKED":
                world.keyCollected()?"EMPTY PRESENT / KEEP SEARCHING FOR VERITY":
                "EMPTY PRESENT / NEXT CHANCE "+std::to_string(World::presentKeyChance(world.presentsOpened()))+"%";
            if(result==PresentResult::Mouse) {
                mouse.releaseFromPresent(nearbyPresent.x,nearbyPresent.z);presentMouseTime=8;
                audio.play(SoundKind::MouseSqueak,{mouse.x,.12,mouse.z},1.f);++mouseSqueaks;
                if(!options.noThreat)partygoers.hearAlarm(mouse.x,mouse.z);
                notice="A MOUSE / THE PARTY HEARD YOU";
                log("Present mouse released / all Partygoers alerted once.");
            }
            if(!world.verityDrops().empty() && !world.infiniteStamina()) {
                const auto& drop=world.verityDrops().back();
                if(std::hypot(drop.x-nearbyPresent.x,drop.z-nearbyPresent.z)<.1) {
                    notice+=" / VERITY! WALK UP TO COLLECT";
                    log("Verity dropped from present.");
                }
            }
            noticeTime=4;
            log("Present opened: "+std::to_string(world.presentsOpened())+(result==PresentResult::Key?" / key found.":result==PresentResult::Mouse?" / mouse.":" / empty."));
        }
    }
    if(options.autoVerity && frame==3 && !world.verityDrops().empty()) {
        const auto drop=world.verityDrops().front();px=drop.x+.5;pz=drop.z;motion.reset();
    }
    if(!input.paused && encounter.alive() && world.collectVerity(px,pz)) {
        stamina.reset();notice="VERITY / INFINITE STAMINA FOR THIS LEVEL";noticeTime=5;
        log("Verity collected. Infinite stamina enabled for Level Fun.");
    }
    HidingSpot nearbyHide{};
    nearHide=world.nearestHidingSpot(px,pz,1.3,nearbyHide);
    if(nearHide) {
        nearHide=Threat::visibleSegment(world,px,pz,nearbyHide.approach.x,nearbyHide.approach.z);
    }
    bool hideInteraction=false;
    if(!input.paused && encounter.alive() && input.hit(GLFW_KEY_E) && (hiding || nearHide)) {
        hideInteraction=true;
        if(hiding) {
            if(!world.blocked(hidingSpot.approach.x,hidingSpot.approach.z,.23) &&
               (!threat.active() || std::hypot(threat.x()-hidingSpot.approach.x,threat.z()-hidingSpot.approach.z)>.7)) {
                px=hidingSpot.approach.x;pz=hidingSpot.approach.z;hiding=false;motion.reset();
                notice="LEFT HIDING PLACE";noticeTime=2;log("Left hiding place.");
            } else { notice="WAIT / ENTRANCE BLOCKED";noticeTime=2; }
        } else if(threat.active() && std::hypot(threat.x()-px,threat.z()-pz)<24 &&
                  Threat::visibleSegment(world,threat.x(),threat.z(),px,pz)) {
            notice="BREAK LINE OF SIGHT BEFORE HIDING";noticeTime=3;
        } else {
            hidingSpot=nearbyHide;hiding=true;px=hidingSpot.inside.x;pz=hidingSpot.inside.z;
            eyeHeight=1.0f;crouched=true;motion.reset();moving=false;frameDistance=0;
            input.setLook(float(std::atan2(hidingSpot.approach.z-hidingSpot.inside.z,hidingSpot.approach.x-hidingSpot.inside.x)*180/3.141592653589793),0);notice.clear();noticeTime=0; log("Entered hiding place.");
        }
    }
    if(hiding) { motion.reset();moving=false;frameDistance=0; }

    nearbyDoor = {};
    nearDoor = world.nearestDoor(px, pz, 2.0, nearbyDoor);
    if (nearDoor) {
        const double dx=nearbyDoor.x-px, dz=nearbyDoor.z-pz;
        const double distance=std::hypot(dx,dz);
        nearDoor = dx*forward.x+dz*forward.z >= .3*distance;
        // Check the approach but stop before the closed panel itself.
        // A nearby handle cannot be used through a perpendicular wall.
        for(double along=.08; nearDoor && along<distance-.35; along+=.08) {
            if(world.blocked(px+dx*along/distance,pz+dz*along/distance,.23,nearbyDoor.id)) nearDoor=false;
        }
    }
    Level0Observation nearbyObservation{};
    nearObservation=!nearDoor&&!nearHide&&!hiding&&world.nearestObservation(px,pz,1.8,nearbyObservation);
    if(nearObservation) {
        const double dx=nearbyObservation.position.x-px,dz=nearbyObservation.position.z-pz;
        const double dy=.09-eyeHeight;
        const double distance=std::sqrt(dx*dx+dy*dy+dz*dz);
        nearObservation=dx*forward.x+dy*forward.y+dz*forward.z>=.78*distance;
    }
    bool observationInteraction=false;
    if(!input.paused && encounter.alive() && nearObservation && input.hit(GLFW_KEY_E)) {
        observationInteraction=true;observationKind=nearbyObservation.kind;observationTime=9;
        ++observationsRead;notice.clear();noticeTime=0;
        audio.play(SoundKind::ClothRustle,{nearbyObservation.position.x,.12,nearbyObservation.position.z},.3f);
        log("Level 0 note read: "+std::string(observationText(observationKind).title));
    }
    if(!input.paused && observationTime>0)observationTime=std::max(0.f,observationTime-dt);
    if (!input.paused && !hiding && !hideInteraction && !observationInteraction && !presentInteraction && !breakerInteraction && (input.hit(GLFW_KEY_E) || (options.autoExit && frame == (options.autoKey ? (options.funPreview?81:21) : options.menuCheck ? 8 : 2)) ||
        (options.autoDoor && frame==2) || (options.autoCloseDoor && frame==options.doorCloseFrame))) {
        if (nearDoor) {
            if (world.toggleDoor(nearbyDoor.id, px, pz)) {
                doorVisualDirty=true;
                ++doorsUsed; doorNoise=1;
                nearbyDoor.open = !nearbyDoor.open;
                audio.play(SoundKind::Door, {nearbyDoor.x, 1.2, nearbyDoor.z});
                notice = nearbyDoor.open ? "OPENING DOOR" : "CLOSING DOOR";
                log(notice + " / " + std::to_string(nearbyDoor.id));
            } else notice = nearbyDoor.isExit && !world.exitUnlocked()?(world.isParking()?"EXIT LOCKED / RESTORE ALL THREE CIRCUITS":"EXIT LOCKED / FIND THE BRASS KEY"):"STEP CLEAR OF THE DOORWAY";
            noticeTime=2;
        } else {
            notice = "FACE A NEARBY DOOR TO INTERACT";
            noticeTime = 3;
        }
    }
    // Actual movement and explicit interaction noise feed one physical actor.
    // Menu time never advances its senses, path following, or grace period.
    const auto previousThreatState=threat.state();
    if(options.autoCatch && frame==2 && encounter.alive() && world.level()==Level::Zero) {
        const Vec3 front=normalize(Vec3{forward.x,0,forward.z});
        double tx=px+front.x*1.35,tz=pz+front.z*1.35;
        if(world.blocked(tx,tz,Threat::RADIUS)) { tx=px; tz=pz; }
        threat.resetAt(tx,tz,std::atan2(-front.x,front.z));
    }
    ThreatPlayer perceivedPlayer{px,pz,forward.x,forward.z,dt>0 ? float(frameDistance/dt) : 0.f,crouched,doorNoise,hiding};
    if(pickedUpKey && !options.noThreat && !options.threatPreview)
        threat.hearAlarm(world,perceivedPlayer,px,pz,true);
    if(options.mouseFixture && frame==2 && mouse.active) {
        px=mouse.x;pz=mouse.z;perceivedPlayer.x=px;perceivedPlayer.z=pz;
    }
    if(world.isFun() && !input.paused && encounter.alive() && presentMouseTime>0) {
        presentMouseTime=std::max(0.f,presentMouseTime-dt);
        if(presentMouseTime==0)mouse.active=false;
    }
    if(mouse.update(world,px,pz,dt,input.paused || !encounter.alive() || (world.isFun() && !mouse.active),hiding,world.level()==Level::Zero)) {
        audio.play(SoundKind::MouseSqueak,{mouse.x,.12,mouse.z},1.f);
        if(!options.noThreat) threat.hearAlarm(world,perceivedPlayer,mouse.x,mouse.z);
        ++mouseSqueaks;log("Mouse squeaked / creature alerted.");
    }
    if(mouse.ambientCue && std::hypot(mouse.x-px,mouse.z-pz)<16) {
        const float transmission=world.soundTransmission(px,pz,mouse.x,mouse.z);
        audio.play(SoundKind::MouseAmbient,{mouse.x,.12,mouse.z},.7f*transmission);
    }
    if(options.mousePreview && frame==1 && mouse.active) {
        px=mouse.x+1;pz=mouse.z+1;
        input.setLook(-135,-48);eyeHeight=1.08f;
    }
    if(options.partyPreview && frame==0 && world.isFun() && !options.noThreat) {
        auto& a=partygoers.actors[0];a.active=true;a.x=px;a.z=pz-3;a.yaw=PI;
    }
    if(options.autoCatch && frame==2 && world.isFun() && !options.noThreat) {
        auto& a=partygoers.actors[0];a.active=true;a.x=px;a.z=pz+.4;
    }
    int gazeWidth=0,gazeHeight=0;glfwGetFramebufferSize(window,&gazeWidth,&gazeHeight);
    partygoers.update(world,px,pz,eyeHeight+bobOffset,forward,fieldOfView,
        float(std::max(1,gazeWidth))/std::max(1,gazeHeight),dt,
        input.paused || !encounter.alive() || options.noThreat);
    if(!input.paused && !options.noThreat && encounter.alive() && partygoers.touchingPlayer(world,px,pz) && encounter.defeatImmediately()) {
        ++catches;motion.reset();moving=false;frameDistance=0;
        setScreen(Screen::Menu);menuSelection=0;notice.clear();noticeTime=0;
        audio.reset(static_cast<std::uint32_t>(world.seed()));music.update(true,true,0,0);
        log("Partygoer contact: game over. Restart at Level Fun checkpoint.");
    }
    if(options.darknessPreview && frame==0 && world.isParking() && !options.noThreat) {
        auto& eyes=darkness.eyes[0];eyes.active=true;eyes.x=px;eyes.z=pz-8;
    }
    if(options.autoCatch && frame==2 && world.isParking() && !options.noThreat) {
        px=3.75;pz=-3.75;world.update(px,pz);
        auto& eyes=darkness.eyes[0];eyes.active=true;eyes.x=px;eyes.z=pz+.4;
    }
    const int beforeDispel=darkness.dispelled,beforeSpawn=darkness.spawned;
    const bool darknessContact=darkness.update(world,px,pz,eyeHeight+bobOffset,forward,flashlight && !hiding,dt,
        input.paused || !encounter.alive() || options.noThreat);
    if(darkness.dispelled>beforeDispel) {
        audio.play(SoundKind::ElectricalStinger,{px,eyeHeight,pz},.16f);
        log("Darkness eyes dispersed by flashlight.");
    }
    if(darkness.spawned>beforeSpawn)for(const auto& eyes:darkness.eyes)if(eyes.active) {
        audio.play(SoundKind::AmbientCreak,{eyes.x,eyes.y,eyes.z},.35f);break;
    }
    if(darknessContact && (encounter.catchPlayer() || encounter.defeatImmediately())) {
        scareYaw=scareTargetYaw=input.yaw;scarePitch=scareTargetPitch=input.pitch;scareApproach=forward;
        ++catches;motion.reset();moving=false;frameDistance=0;
        setScreen(Screen::Menu);menuSelection=0;notice.clear();noticeTime=0;
        audio.reset(static_cast<std::uint32_t>(world.seed()));music.update(true,true,0,0);
        if(scareConfig.audio && encounter.presenting()) {
            audio.play(SoundKind::DarknessJumpscare,{px,eyeHeight,pz},1.f);
            log("Darkness sound: electrical shriek.");
        }
        log(encounter.presenting()?"Darkness contact: jumpscare started.":"Darkness contact: game over (jumpscare disabled).");
    }
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
                    << std::fixed << std::setprecision(2) << std::hypot(threat.x()-px,threat.z()-pz) << " m";
        log(threatEvent.str());
    }
    // Visual acquisition also fires when a sound already started the chase;
    // a held sight line repeats it. Hearing alone never cues a notice.
    const bool threatCanVocalize=world.level()==Level::Zero && threat.active()&&!threat.caught()&&!options.noThreat;
    for(const auto cue:threatVocals.update(dt,threatCanVocalize,
        threatCanVocalize&&threat.canSeePlayer()&&!hiding,false,
        input.paused||options.threatPreview,!options.noThreat&&encounter.alive()&&world.level()==Level::Zero)) {
        audio.play(cue,{threat.x(),1.8,threat.z()},1.f);
        if(cue==SoundKind::ThreatNotice)++threatNotices;else ++threatRoars;
        log(cue==SoundKind::ThreatNotice?"Threat audio cue: visual notice.":"Threat audio cue: rage roar.");
    }
    encounter.update(dt,scareSuspended);
    const bool debugScare=world.level()==Level::Zero && input.hit(GLFW_KEY_J) && encounter.alive() && !flow.is(Screen::Settings);
    if(debugScare && !threat.active()) {
        threat.resetAt(px+forward.x*2,pz+forward.z*2);
    }
    const float scareDistance=float(std::hypot(threat.x()-px,threat.z()-pz));
    const bool nearScare=!hiding && !input.paused && !options.noThreat && threat.active() &&
        encounter.shouldTrigger(scareDistance,
            scareDistance<=scareConfig.triggerDistance && Threat::visibleSegment(world,px,pz,threat.x(),threat.z()),threat.caught());
    if((debugScare || nearScare) && encounter.catchPlayer()) {
        scareYaw=input.yaw; scarePitch=input.pitch;
        const float cy=std::cos(threatBodyYaw),sy=std::sin(threatBodyYaw);
        const Vec3 toHead{float(threat.x()-px)+threatHeadLocal.x*cy-threatHeadLocal.z*sy,
            threatHeadLocal.y-eyeHeight-bobOffset,float(threat.z()-pz)+threatHeadLocal.x*sy+threatHeadLocal.z*cy};
        scareApproach=dot(toHead,toHead)>.01f?normalize(toHead):forward;
        scareTargetYaw=std::atan2(scareApproach.z,scareApproach.x)*180/PI;
        scareTargetPitch=std::asin(std::clamp(scareApproach.y,-1.f,1.f))*180/PI;
        ++catches; motion.reset(); moving=false; frameDistance=0;threatFootPlanting.reset();
        setScreen(Screen::Menu); menuSelection=0;
        notice.clear(); noticeTime=0;
        music.update(true,true,0,0);
        audio.reset(static_cast<std::uint32_t>(world.seed()+catches));
        if(scareConfig.audio) audio.play(SoundKind::Jumpscare,{px,eyeHeight,pz},1.f);
        log("Captured: 3D jumpscare started.");
    }
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
                if(threat.canSeePlayer())pitchTarget=std::atan2(eyeHeight-THREAT_EYE_HEIGHT,float(std::max(1.,std::hypot(px-threat.x(),pz-threat.z()))));
            } else if(threat.canSeePlayer()) {
                headTarget=std::remainder(float(std::atan2(px-threat.x(),-(pz-threat.z())))-threatBodyYaw,2*PI);
                pitchTarget=std::atan2(eyeHeight-THREAT_EYE_HEIGHT,float(std::max(1.,std::hypot(px-threat.x(),pz-threat.z()))));
            } else headTarget=std::sin(threatAnimation*.65f)*.32f;
            headTarget=std::clamp(headTarget,-.8f,.8f);
            pitchTarget=std::clamp(pitchTarget,-.25f,.20f);
            if(threat.speed()>.15f)threatHeadYaw=0;
            else threatHeadYaw+=std::clamp((headTarget-threatHeadYaw)*(1-std::exp(-dt*8.f)),-dt*2.5f,dt*2.5f);
            threatHeadPitch+=(pitchTarget-threatHeadPitch)*(1-std::exp(-dt*7.f));
        }
    }

    return true;
}

} // namespace br::app
