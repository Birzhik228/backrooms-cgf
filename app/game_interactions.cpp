#include "game_internal.h"

namespace br::app {

Game::InteractionResult Game::updateInteractions() {
    applyInteractionScenario();
    const bool pickedUpKey=collectLevelKey();
    const bool breakerInteraction=interactWithBreaker();
    const bool presentInteraction=interactWithPresent();
    applyVerityScenario();
    const bool verityInteraction=collectVerity(presentInteraction || breakerInteraction);
    const bool hideInteraction=interactWithHidingPlace(verityInteraction);
    updateNearbyDoor();
    const bool observationInteraction=readObservation();
    const bool otherInteraction=hideInteraction || observationInteraction || presentInteraction || breakerInteraction || verityInteraction;
    const float doorNoise=interactWithDoor(otherInteraction);
    return {pickedUpKey,doorNoise};
}

bool Game::collectLevelKey() {
    const bool pickedUpKey=!input.paused && encounter.alive() && !player.hiding && world.collectKey(player.x,player.z);
    if(pickedUpKey) {
        notice="KEY FOUND / EXIT UNLOCKED / IT HEARD YOU";noticeTime=5;
        log("Key collected. Exit unlocked; press E to open. Creature called once.");
    }
    return pickedUpKey;
}

bool Game::interactWithBreaker() {
    nearbyPresent = {};
    nearbyBreaker=-1;
    nearBreaker=world.nearestBreaker(player.x,player.z,1.8,nearbyBreaker);
    if(nearBreaker) {
        const auto p=world.breakerLocation(nearbyBreaker);
        nearBreaker=(p.x-player.x)*forward.x+(p.z-player.z)*forward.z>.35*std::hypot(p.x-player.x,p.z-player.z);
    }
    const bool breakerInteraction=nearBreaker && !input.paused && encounter.alive() && input.hit(GLFW_KEY_E);
    if(breakerInteraction && world.activateBreaker(nearbyBreaker,player.x,player.z)) {
        notice="CIRCUIT "+std::string(1,char('A'+nearbyBreaker))+" ONLINE / SECTOR SAFE / "+std::to_string(world.breakersPowered())+" OF 3";
        if(world.exitUnlocked())notice+=" / SERVICE EXIT UNLOCKED";
        noticeTime=5;log(notice);nearBreaker=false;
    }
    return breakerInteraction;
}

bool Game::interactWithPresent() {
    nearPresent=world.nearestPresent(player.x,player.z,1.8,nearbyPresent);
    if(nearPresent) {
        const double dx=nearbyPresent.x-player.x,dz=nearbyPresent.z-player.z,d=std::hypot(dx,dz);
        nearPresent=facingPresent(nearbyPresent,player.x,player.z,player.eyeHeight+player.bobOffset,forward);
        for(double along=.05;nearPresent && along<d-.46;along+=.05)
            if(world.blocked(player.x+dx*along/d,player.z+dz*along/d,.05))nearPresent=false;
    }
    const bool presentInteraction=nearPresent && !input.paused && encounter.alive() && input.hit(GLFW_KEY_E);
    if(presentInteraction) {
        const auto result=world.openPresent(nearbyPresent.id,player.x,player.z);
        if(result!=PresentResult::Invalid) {
            nearbyPresent.opened=true;
            audio.play(SoundKind::PresentOpen,{nearbyPresent.x,.4,nearbyPresent.z},1.f);
            nearPresent=false;
            notice=result==PresentResult::Key?"KEY FOUND / EXIT UNLOCKED":
                world.keyCollected()?"EMPTY PRESENT":
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
                    log("Verity dropped from present.");
                }
            }
            noticeTime=4;
            log("Present opened: "+std::to_string(world.presentsOpened())+(result==PresentResult::Key?" / key found.":result==PresentResult::Mouse?" / mouse.":" / empty."));
        }
    }
    return presentInteraction;
}

bool Game::collectVerity(bool otherInteraction) {
    WorldPoint nearbyVerity{};
    nearVerity=world.nearestVerity(player.x,player.z,1.2,nearbyVerity);
    if(nearVerity) {
        const Vec3 direction{float(nearbyVerity.x-player.x),.18f-player.eyeHeight,float(nearbyVerity.z-player.z)};
        nearVerity=dot(normalize(direction),forward)>.75f;
    }
    const bool verityInteraction=nearVerity && !otherInteraction &&
        !input.paused && encounter.alive() && input.hit(GLFW_KEY_E);
    if(verityInteraction && world.collectVerity(player.x,player.z)) {
        audio.play(SoundKind::VerityPickup,{player.x,player.eyeHeight,player.z},1.f);
        veritySongTime=2.f;
        log("Verity pickup song triggered.");
        nearVerity=false;
        player.stamina.reset();notice="VERITY / INFINITE STAMINA FOR THIS LEVEL";noticeTime=5;
        log("Verity collected. Infinite stamina enabled for Level Fun.");
    }
    return verityInteraction;
}

bool Game::interactWithHidingPlace(bool otherInteraction) {
    HidingSpot nearbyHide{};
    nearHide=world.nearestHidingSpot(player.x,player.z,1.3,nearbyHide);
    if(nearHide) {
        nearHide=Threat::visibleSegment(world,player.x,player.z,nearbyHide.approach.x,nearbyHide.approach.z);
    }
    bool hideInteraction=false;
    if(!otherInteraction && !input.paused && encounter.alive() && input.hit(GLFW_KEY_E) && (player.hiding || nearHide)) {
        hideInteraction=true;
        if(player.hiding) {
            if(!world.blocked(player.hidingSpot.approach.x,player.hidingSpot.approach.z,.23) &&
               (!threat.active() || std::hypot(threat.x()-player.hidingSpot.approach.x,threat.z()-player.hidingSpot.approach.z)>.7)) {
                player.x=player.hidingSpot.approach.x;player.z=player.hidingSpot.approach.z;player.hiding=false;player.motion.reset();
                notice="LEFT HIDING PLACE";noticeTime=2;log("Left hiding place.");
            } else { notice="WAIT / ENTRANCE BLOCKED";noticeTime=2; }
        } else if(threat.active() && std::hypot(threat.x()-player.x,threat.z()-player.z)<24 &&
                  Threat::visibleSegment(world,threat.x(),threat.z(),player.x,player.z)) {
            notice="BREAK LINE OF SIGHT BEFORE HIDING";noticeTime=3;
        } else {
            player.hidingSpot=nearbyHide;player.hiding=true;player.x=player.hidingSpot.inside.x;player.z=player.hidingSpot.inside.z;
            player.eyeHeight=1.0f;player.crouched=true;player.motion.reset();moving=false;frameDistance=0;
            input.setLook(float(std::atan2(player.hidingSpot.approach.z-player.hidingSpot.inside.z,player.hidingSpot.approach.x-player.hidingSpot.inside.x)*180/3.141592653589793),0);notice.clear();noticeTime=0; log("Entered hiding place.");
        }
    }
    if(player.hiding) { player.motion.reset();moving=false;frameDistance=0; }
    return hideInteraction;
}

void Game::updateNearbyDoor() {
    nearbyDoor = {};
    nearDoor = world.nearestDoor(player.x, player.z, 2.0, nearbyDoor);
    if (nearDoor) {
        const double dx=nearbyDoor.x-player.x, dz=nearbyDoor.z-player.z;
        const double distance=std::hypot(dx,dz);
        nearDoor = dx*forward.x+dz*forward.z >= .3*distance;
        // Check the approach but stop before the closed panel itself.
        // A nearby handle cannot be used through a perpendicular wall.
        for(double along=.08; nearDoor && along<distance-.35; along+=.08) {
            if(world.blocked(player.x+dx*along/distance,player.z+dz*along/distance,.23,nearbyDoor.id)) nearDoor=false;
        }
    }
}

bool Game::readObservation() {
    Level0Observation nearbyObservation{};
    nearObservation=!nearDoor&&!nearHide&&!player.hiding&&world.nearestObservation(player.x,player.z,1.8,nearbyObservation);
    if(nearObservation) {
        const double dx=nearbyObservation.position.x-player.x,dz=nearbyObservation.position.z-player.z;
        const double dy=.09-player.eyeHeight;
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
    return observationInteraction;
}

float Game::interactWithDoor(bool otherInteraction) {
    float doorNoise=0;
    if (!input.paused && !player.hiding && !otherInteraction && doorInteractionRequested()) {
        if (nearDoor) {
            if (world.toggleDoor(nearbyDoor.id, player.x, player.z)) {
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
    return doorNoise;
}

} // namespace br::app
