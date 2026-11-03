#include "game_internal.h"

namespace br::app {
namespace fs = std::filesystem;

void Game::drawHud() {
    const float scale = std::max(.85f, std::min(float(width) / 1280, float(height) / 800));
    const Color paper{.88f, .87f, .73f, 1};
    const Color gold{.85f, .72f, .39f, 1};
    const Color dim{.57f, .58f, .46f, 1};
    if (cameraView && !input.paused) {
        ui.rect(30, 29, 7, 7, {.72f, .23f, .13f, .85f});
        ui.text(47, 28, "REC", 1.35f * scale, paper);
        std::ostringstream tape;
        tape << std::setfill('0') << std::setw(2) << (int(elapsed) / 60) % 60
             << ":" << std::setw(2) << int(elapsed) % 60;
        ui.text(float(width) - 105, 28, tape.str(), 1.35f * scale, paper);
    }
    if ((!cameraView || input.paused) && !encounter.scaring()) {
        ui.rect(18, 18, 330 * scale, 50, {.015f, .020f, .015f, .70f});
        ui.rect(22, 24, 3, 38, gold);
        ui.text(36, 25, "BACKROOMS", 2 * scale, paper);
        ui.text(36, 47, world.isParking()?std::string("STAGE 3 / PARKING / SECTOR ")+char('A'+parkingSector(int64_t(std::floor(player.x/CELL)),int64_t(std::floor(player.z/CELL)))):world.isFun()?std::string("LEVEL FUN / PARTY ROOMS"):std::string("LEVEL 0 / ")+roomName(currentRoom.kind), 1.15f * scale, dim);
        const std::string seedText = "SEED " + std::to_string(world.seed());
        ui.text(float(width) - UI::textWidth(seedText, 1.25f * scale) - 26,
                28, seedText, 1.25f * scale, paper);

        if (!input.paused) {
            ui.rect(width * .5f - 1, height * .5f - 1, 2, 2, {.95f, .94f, .81f, .65f});
            ui.text(28, float(height) - 55,
                    "FLASHLIGHT " + std::string(flashlight && !player.hiding ? "ON" : "OFF") +
                    (world.isParking()?std::string(""):std::string("   CLUES ") + (clues ? "ON" : "OFF")) +
                    (player.crouched ? "   CROUCHING" : ""), 1.2f * scale, paper);
            {
                const auto door = world.exitLocation();
                const double dx = door.x - player.x, dz = door.z - player.z;
                const double distance = std::hypot(dx, dz);
                const double ahead = dx * forward.x + dz * forward.z;
                const double side = -dx * forward.z + dz * forward.x;
                const std::string bearing = ahead > distance * .75 ? "AHEAD" :
                    ahead < -distance * .75 ? "BEHIND" : side > 0 ? "RIGHT" : "LEFT";
                const std::string signal = distance < 10000 ? std::to_string(int(distance)) + " M / " + bearing : "DISTANT / " + bearing;
                ui.text(28, float(height) - 80, "EXIT SIGNAL " + signal, 1.25f * scale, {.47f, .88f, .68f, 1});
                auto key=world.keyLocation();
                int signalCircuit=-1;
                if(world.isParking()) {
                    signalCircuit=parkingCircuitTarget(world,player.x,player.z);
                    if(signalCircuit>=0)key=world.breakerLocation(signalCircuit);
                    for(int i=0;i<3;++i) {
                        const float bx=28+i*84*scale,by=float(height)-160;
                        const Color state=world.breakerPowered(i)?Color{.47f,.88f,.68f,1}:gold;
                        ui.rect(bx,by,78*scale,23,{.025f,.035f,.03f,.85f});
                        ui.rect(bx,by,78*scale,2,i==signalCircuit?paper:state);
                        ui.text(bx+7*scale,by+8,std::string(1,char('A'+i))+(world.breakerPowered(i)?" ON":" OFF"),1.15f*scale,state);
                    }
                }
                const double kx=key.x-player.x,kz=key.z-player.z,kd=std::hypot(kx,kz);
                const double ka=kx*forward.x+kz*forward.z,ks=-kx*forward.z+kz*forward.x;
                const double headingScale=world.isParking()?std::hypot(forward.x,forward.z):1.;
                const std::string kb=ka>kd*headingScale*.75?"AHEAD":ka<-kd*headingScale*.75?"BEHIND":ks>0?"RIGHT":"LEFT";
                ui.text(28,float(height)-104,world.isParking()?
                    "POWER "+std::to_string(world.breakersPowered())+"/3 / "+(signalCircuit<0?std::string("SERVICE EXIT READY / E TO OPEN"):
                        "CIRCUIT "+std::string(1,char('A'+signalCircuit))+" / "+std::to_string(int(kd))+" M / "+kb):
                    world.keyCollected()?"KEY COLLECTED / EXIT UNLOCKED / E TO OPEN":
                    world.isFun()?"SEARCH PRESENTS / OPENED "+std::to_string(world.presentsOpened())+" / NEXT "+std::to_string(World::presentKeyChance(world.presentsOpened()))+"%":
                    "KEY SIGNAL "+std::to_string(int(kd))+" M / "+kb,1.25f*scale,gold);
            }
            if(world.isParking())ui.text(28,float(height)-128,Darkness::safe(world,player.x,player.z)?"IN THE LIGHT / SAFE":"DARKNESS / AIM YOUR FLASHLIGHT AT THE EYES",1.15f*scale,Darkness::safe(world,player.x,player.z)?Color{.47f,.88f,.68f,1}:gold);
            if(world.isFun() && clues && !world.keyCollected()) {
                const auto gift=funPresentClue(world,player.x,player.z,true);
                std::string signal="PRESENT SIGNAL / EXPLORE NEARBY ROOMS";
                if(gift) {
                    const double dx=gift->x-player.x,dz=gift->z-player.z,d=std::hypot(dx,dz);
                    const double ahead=dx*forward.x+dz*forward.z,side=-dx*forward.z+dz*forward.x;
                    const double heading=std::hypot(forward.x,forward.z);
                    const char* bearing=ahead>d*heading*.75?"AHEAD":ahead<-d*heading*.75?"BEHIND":side>0?"RIGHT":"LEFT";
                    signal="PRESENT SIGNAL "+std::to_string(int(std::ceil(d)))+" M / "+bearing;
                }
                ui.text(28,float(height)-128,signal,1.15f*scale,{.94f,.56f,.72f,1});
            }
            if(nearBreaker)ui.text(width*.5f-170,height*.5f+58,"E / RESTORE CIRCUIT "+std::string(1,char('A'+nearbyBreaker)),1.3f*scale,paper);
            if(nearPresent) ui.text(width*.5f-160,height*.5f+58,
                nearbyPresent.opened?"PRESENT ALREADY SEARCHED":"E / OPEN PRESENT",1.3f*scale,paper);
            if(nearObservation && observationTime<=0)ui.text(width*.5f-100,height*.5f+58,
                "E / READ NOTE",1.3f*scale,paper);
            if(player.hiding || nearHide) ui.text(width*.5f-170,height*.5f+58,
                player.hiding?"HIDDEN / E TO LEAVE":"E / HIDE IN WALL RECESS",1.3f*scale,paper);
            ui.text(float(width)-212, float(height)-70, world.infiniteStamina()?"VERITY / INFINITE":player.stamina.exhausted ? "CATCH YOUR BREATH" : "STAMINA", 1.15f*scale, player.stamina.exhausted ? gold : dim);
            ui.rect(float(width)-212, float(height)-49, 180, 5, {.08f,.09f,.06f,.9f});
            ui.rect(float(width)-212, float(height)-49, 180*player.stamina.value/100.f, 5, gold);
            if (nearDoor) {
                const std::string prompt = nearbyDoor.isExit && !world.exitUnlocked()?(world.isParking()?"EXIT LOCKED / RESTORE POWER":"EXIT LOCKED / FIND KEY") : nearbyDoor.isExit && nearbyDoor.openness>.92f ? "WALK THROUGH TO EXIT / E CLOSE" : nearbyDoor.open ? "E / CLOSE DOOR" : nearbyDoor.isExit ? "E / OPEN EXIT DOOR" : "E / OPEN DOOR";
                const float promptW = UI::textWidth(prompt, 1.8f * scale);
                ui.rect(width * .5f - promptW * .5f - 16, height * .63f - 12, promptW + 32, 38, {.025f, .1f, .065f, .94f});
                ui.text(width * .5f - promptW * .5f, height * .63f, prompt, 1.8f * scale, {.65f, 1, .8f, 1});
            }
        }
    }
    if(observationTime>0 && !input.paused && encounter.alive() && !cameraView)
        drawObservation(ui,width,height,observationKind);
    if (showMap && !input.paused) {
        minimap(ui, world, player.x, player.z, forward, width, clues);
    }
    if(settings.showFPS && !input.paused) {
        const std::string rate="FPS "+std::to_string(int(fps));
        const float w=UI::textWidth(rate,1.4f);
        ui.rect(float(width)-w-38,66,w+20,25,{.02f,.025f,.02f,.82f});
        ui.text(float(width)-w-28,74,rate,1.4f,paper);
    }
    if (diagnostics && !encounter.scaring()) {
        std::ostringstream text;
        text << "FPS " << int(fps)
             << "\nRESIDENT TRIANGLES " << residentTriangles << " / DRAWN " << drawnTriangles
             << "\nCHUNKS " << world.chunks().size() << " / DRAWN " << drawn
             << "\nCELL " << int64_t(std::floor(player.x / CELL)) << ", " << int64_t(std::floor(player.z / CELL))
             << "\nDISTANCE " << int(player.travelDistance) << " M / SHIFTS " << world.shifts()
             << "\nLIGHTS " << lightCount << " / SPEED " << std::fixed << std::setprecision(2) << player.motion.speed()
             << "\nROOM " << currentRoom.widthCells * CELL << " X " << currentRoom.depthCells * CELL
             << " M / CEILING " << currentRoom.height << " M"
             << "\nEYE " << std::fixed << std::setprecision(2) << player.eyeHeight << " M / " << (player.crouched ? "CROUCH" : "STAND")
             << "\nTHREAT " << threatStateName(threat.state()) << " / PATH NODES " << threat.pathNodesVisited();
        text << "\nPROCESS RAM " << (performance.memory().residentKnown?
                    std::to_string(performance.memory().resident/1048576)+" MIB":std::string("N/A"))
             << " / GL OBJECTS " << performance.liveResources()
             << "\nFRAME P95 " << performance.statistics().frames().percentile(.95)
             << " MS / SLOW " << performance.statistics().slowCount();
        ui.rect(26, 83, 520, 220, {.02f, .025f, .02f, .82f});
        ui.text(38, 95, text.str(), 1.4f, paper);
    }
    if (noticeTime > 0) {
        noticeTime -= dt;
        const bool besideMap=world.isParking() && showMap && !input.paused;
        const float noticeArea=besideMap?float(width)-306:float(width)-48;
        const float noticeScale=std::min(1.5f,noticeArea/std::max(1.f,UI::textWidth(notice,1)));
        const float textWidth = UI::textWidth(notice, noticeScale);
        const float noticeCenter=besideMap?28+noticeArea*.5f:width*.5f;
        ui.rect(noticeCenter - textWidth * .5f - 12, 84, textWidth + 24, 30,
                {.02f, .025f, .02f, .85f});
        ui.text(noticeCenter - textWidth * .5f, 94, notice, noticeScale, gold);
    }
    if (input.paused && !encounter.scaring() && !flow.is(Screen::Controls) && !flow.is(Screen::LevelSelect)) {
        ui.rect(0,0,float(width),float(height),{.014f,.018f,.011f,.70f});
        const float boxW=std::min(700.f,float(width)-40), boxH=550;
        const float left=(width-boxW)*.5f, top=(height-boxH)*.5f;
        ui.rect(left,top,boxW,boxH,{.032f,.039f,.027f,.96f});
        ui.rect(left,top,boxW,2,gold);
        ui.text(left+32,top+27,flow.is(Screen::Settings) ? "CAMERA / SOUND / DISPLAY" : world.isParking()?"STAGE 3 / UNDERGROUND PARKING":world.isFun()?"LEVEL FUN / PARTY ROOMS":"LEVEL 0 / ENDLESS EXPLORATION",1.4f,gold);
        ui.text(left+32,top+57,flow.is(Screen::Settings) ? "SETTINGS" : encounter.gameOver() ? "GAME OVER" : flow.escaped ? "PARKING COMPLETE" : flow.entered ? "PAUSED" : "BACKROOMS",3.6f,paper);
        if (flow.is(Screen::Settings)) {
            ui.text(left+boxW-245,top+69,"LIVE FPS "+std::to_string(int(fps)),1.6f,gold);
            const std::array<std::string,14> labels={"MOUSE SENSITIVITY","MASTER VOLUME","BRIGHTNESS","FIELD OF VIEW","HEAD BOB","CAMERA SOFTNESS","VSYNC","MUSIC","MUSIC VOLUME","CONTACT SHADING","FPS COUNTER","FPS LIMIT","RESET DEFAULTS","BACK"};
            auto decimal=[](float value,int precision) {std::ostringstream out;out<<std::fixed<<std::setprecision(precision)<<value;return out.str();};
            const std::array<std::string,12> values={decimal(settings.sensitivity,3),std::to_string(int(settings.volume*100+.5f))+"%",std::to_string(int(settings.brightness*100+.5f))+"%",std::to_string(int(settings.fov)),std::to_string(int(settings.headBob*100+.5f))+"%",std::to_string(int(settings.blur*100+.5f))+"%",settings.vsync?"ON":"OFF",settings.musicEnabled?"ON":"OFF",std::to_string(int(settings.musicVolume*100+.5f))+"%",std::to_string(int(settings.ambientOcclusion*100+.5f))+"%",settings.showFPS?"ON":"OFF",settings.maxFps?std::to_string(settings.maxFps):"UNLIMITED"};
            for(int i=0;i<14;++i) {
                float y=top+112+i*25;
                const bool selected=i==settingSelection;
                ui.rect(left+28,y,boxW-56,25,selected ? Color{.19f,.21f,.13f,1} : Color{.048f,.055f,.037f,1});
                ui.text(left+42,y+10,labels[i],1.5f,selected?gold:paper);
                if(i<12) {
                    ui.text(left+boxW-260,y+10,values[i],1.4f,paper);
                    ui.text(left+boxW-126,y+9,"-",1.8f,gold);
                    ui.text(left+boxW-65,y+9,"+",1.8f,gold);
                }
            }
            ui.text(left+32,top+boxH-68,music.available() ? "MUSIC: LOADED" : "MUSIC: NO TRACK LOADED",1.1f,dim);
            ui.text(left+32,top+boxH-47,"UP/DOWN SELECT   LEFT/RIGHT ADJUST   ESC BACK",1.3f,dim);
            ui.text(left+32,top+boxH-25,"CLICK A ROW OR ITS - / + CONTROLS. SETTINGS SAVE AUTOMATICALLY.",1.1f,dim);
        } else {
            ui.text(left+32,top+107,encounter.gameOver() ? (world.isParking()?"THE DARKNESS FOUND YOU. AIM YOUR FLASHLIGHT AT THE EYES.":world.isFun()?"CAUGHT AT THE PARTY. RESTART FROM THE LEVEL FUN CHECKPOINT.":"IT FOUND YOU. BREAK SIGHT. CROUCH TO STAY QUIET.") : flow.escaped ? "PARKING COMPLETE. THE SERVICE EXIT IS POWERED. YOU MADE IT." : world.isParking()?"LIGHT REPELS THE EYES. RESTORE A, B AND C TO ESCAPE.":world.isFun()?"SEARCH PRESENTS FOR THE KEY. KEEP PARTYGOERS IN SIGHT.":"FOLLOW THE HUM. LISTEN FOR FOOTSTEPS THAT ARE NOT YOURS.",1.35f,dim);
            const std::array<std::string,6> labels={encounter.gameOver()?(world.level()!=Level::Zero?"RESTART CHECKPOINT":"RESTART RUN"):flow.escaped?"CONTINUE EXPLORING":flow.entered?"RESUME":"ENTER THE BACKROOMS","SETTINGS","CHOOSE A LEVEL","NEW SEEDED BUILDING",encounter.gameOver()?"MAIN MENU":"RETURN TO FIRST ROOM","QUIT"};
            for(int i=0;i<6;++i) {
                float y=top+163+i*48; const bool selected=i==menuSelection;
                ui.rect(left+32,y,boxW-64,43,selected ? Color{.69f,.59f,.31f,1} : Color{.08f,.09f,.058f,1});
                ui.text(left+50,y+14,labels[i],1.7f,selected?Color{.055f,.065f,.035f,1}:paper);
            }
            ui.text(left+32,top+472,encounter.gameOver()?"CLICK RESTART OR PRESS ENTER   O SETTINGS   Q QUIT":"CLICK OR UP/DOWN + ENTER   O SETTINGS   ESC RESUME",1.2f,dim);
            ui.text(left+32,top+boxH-27,"F9 / PREVIEW ENDING    CTRL / CROUCH",1.1f,dim);
        }
    }
    if(flow.is(Screen::Controls)) drawControlsGuide(ui,width,height,guidePage,world.isFun(),world.isParking());
    if(flow.is(Screen::LevelSelect))drawLevelMenu(ui,width,height,levelMenuSelection,int(world.level()));
    ui.render(width, height);
}

} // namespace br::app
