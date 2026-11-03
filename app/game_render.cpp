#include "game_internal.h"

namespace br::app {
namespace fs = std::filesystem;

Vec3 Game::headCenter(const std::vector<Vertex>& vertices) {
    Vec3 low{100,100,100},high{-100,-100,-100};bool found=false;
    for(const auto& v:vertices)if(v.material==THREAT_EYE_MATERIAL) {
        found=true;low={std::min(low.x,v.x),std::min(low.y,v.y),std::min(low.z,v.z)};
        high={std::max(high.x,v.x),std::max(high.y,v.y),std::max(high.z,v.z)};
    }
    return found?Vec3{(low.x+high.x)*.5f,low.y+(high.y-low.y)*.7f,(low.z+high.z)*.5f}:Vec3{0,THREAT_EYE_HEIGHT,0};
}

bool Game::renderWorld() {
    const double uploadStart = glfwGetTime();
    uploads = 0;
    for (auto it = gpu.begin(); it != gpu.end();) {
        if (world.chunks().count(it->first) == 0 && world.preparedChunks().count(it->first) == 0) {
            release(it->second);
            it = gpu.erase(it);
        } else {
            ++it;
        }
    }
    residentTriangles = 0;
    for (const auto& [coord, chunk] : world.chunks()) {
        auto& mesh = gpu[coord];
        if (!mesh.vao || mesh.revision != chunk.revision) {
            upload(mesh, chunk);
            ++uploads;
        }
        residentTriangles += mesh.count / 3;
    }
    // Upload one prepared outer-ring mesh ahead of travel. Promotion into
    // the resident set retains its revision and needs no new GL transfer.
    for (const auto& [coord, chunk] : world.preparedChunks()) {
        auto& mesh = gpu[coord];
        if (!mesh.vao || mesh.revision != chunk.revision) {
            upload(mesh, chunk);
            ++uploads;
            break;
        }
    }

    if(threat.active() && (!input.paused || lastThreatMeshTime<0)) {
        auto stoopFor=[](float clearance) { return clearance<2.8f ? 1.f : clearance<3.f ? .5f : 0.f; };
        // The four-legged silhouette is wider than its navigation body.
        // Probe its swept footprint as well as the ceiling so knees tuck
        // beside pillars and full-height walls, not only at door lintels.
        const float cy=std::cos(threatBodyYaw),sy=std::sin(threatBodyYaw);
        bool narrow=false;
        for(float side:{-1.4f,1.4f})for(float along:{-1.2f,1.8f}) {
            if(narrow)break;
            const double dx=side*cy-along*sy,dz=side*sy+along*cy;
            for(int step=1;step<=6;++step) {
                const double t=step/6.;
                if(world.blocked(threat.x()+dx*t,threat.z()+dz*t,.08)) {narrow=true;break;}
            }
        }
        const float ahead=std::max(narrow?1.f:0.f,stoopFor(world.headClearance(threat.x(),threat.z(),1.75)));
        const float minimum=stoopFor(world.headClearance(threat.x(),threat.z(),1.0));
        threatStoop+=(ahead-threatStoop)*(1.f-std::exp(-dt*9.f));
        threatStoop=std::max(threatStoop,minimum);
    }
    if(threat.active() && (lastThreatMeshTime<0 || threatAnimation-lastThreatMeshTime>=1.f/60.f ||
                          std::abs(threatStoop-lastThreatStoop)>.002f || lastScareMesh!=encounter.scaring())) {
        const float visualSpeed=options.threatPreview ? options.threatPreviewSpeed : threatVisualSpeed;
        threatFootPlanting.setRoot(threat.x(),threat.z(),threatBodyYaw,threatTurnWeight);
        const auto posedCreature=threatModelVertices(options.threatPreview ? options.threatPreviewTime : threatAnimation,visualSpeed,
            visualSpeed>2.f,encounter.scaring() ? std::max(.001f,encounter.progress()) : 0.f,threatStoop,
            options.threatPreview ? -1.f : threatGaitPhase,
            options.threatPreview ? options.threatPreviewHeadYaw*PI/180 : threatHeadYaw,
            options.threatPreview ? options.threatPreviewHeadPitch*PI/180 : threatHeadPitch,
            options.threatPreview?nullptr:&threatFootPlanting);
        uploadVertices(threatGpu,posedCreature,GL_DYNAMIC_DRAW);
        if(encounter.alive())threatHeadLocal=headCenter(posedCreature);
        lastThreatMeshTime=threatAnimation; lastScareMesh=encounter.scaring(); lastThreatStoop=threatStoop;
    }
    if(threat.active()) residentTriangles+=threatGpu.count/3;


    uploadMs = (glfwGetTime() - uploadStart) * 1000;
    const double setupStart = glfwGetTime();
    width = height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0) {
        glfwWaitEventsTimeout(.02);
        return false;
    }
    target.resize(width, height);
    ambientTarget.resize(width,height);
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Subtract a double-precision chunk origin before converting to float.
    // Camera, geometry, and light positions share this local coordinate system.
    origin = chunkAt(px, pz);
    const double originX = origin.x * CHUNK_SIZE;
    const double originZ = origin.z * CHUNK_SIZE;
    std::array<double,Partygoers::COUNT*5> partyPose{};
    for(int i=0;i<Partygoers::COUNT;++i) {
        const auto& a=partygoers.actors[i];
        partyPose[i*5]=a.active?1:0;partyPose[i*5+1]=a.x-originX;partyPose[i*5+2]=a.z-originZ;
        partyPose[i*5+3]=a.yaw;partyPose[i*5+4]=a.phase;
    }
    if(partyPose!=lastPartyPose) {
        uploadVertices(partyGpu,partygoerVertices(partygoers,originX,originZ),GL_DYNAMIC_DRAW);
        lastPartyPose=partyPose;
    }
    residentTriangles+=partyGpu.count/3;
    if(world.isParking() || darknessGpu.count)uploadVertices(darknessGpu,darkness.vertices(originX,originZ,px,pz),GL_DYNAMIC_DRAW);
    residentTriangles+=darknessGpu.count/3;
    if(doorVisualDirty || !(origin==doorOrigin) || doorShift!=world.shifts()) {
        uploadVertices(doorGpu,world.doorVertices(originX,originZ,presence.doorId(),presence.handleAngle()),GL_DYNAMIC_DRAW);
        doorOrigin=origin;doorShift=world.shifts();doorVisualDirty=false;
    }
    residentTriangles+=doorGpu.count/3;
    const float stride = crouched ? .90f : sprinting ? 1.85f : 1.50f;
    const float phase = float(std::fmod(std::max(0.0, walk - .20), stride)) * (2 * PI / stride);
    const float weight = moving ? settings.headBob * (crouched ? .010f : sprinting ? .035f : .028f) : 0.f;
    const float bobTarget = -weight * (.5f + .5f * std::cos(phase));
    bobOffset += (bobTarget - bobOffset) * (1.f - std::exp(-14.f * dt));
    Vec3 eye{float(px - originX), eyeHeight + bobOffset, float(pz - originZ)};
    const Vec3 stableEye=eye;
    cameraForward=forward;
    shakeRoll=0;
    scareShake=encounter.shake();
    if(encounter.presenting()) {
        const float turn=encounter.cameraTurn();
        const float yaw=(scareYaw+std::remainder(scareTargetYaw-scareYaw,360.f)*turn)*PI/180;
        const float pitch=(scarePitch+(scareTargetPitch-scarePitch)*turn)*PI/180;
        cameraForward={std::cos(yaw)*std::cos(pitch),std::sin(pitch),std::sin(yaw)*std::cos(pitch)};
        eye=eye+Vec3{scareShake.x,scareShake.y,scareShake.z};
        shakeRoll=scareShake.roll;
    }

    Mat4 roll=Mat4::identity();
    roll.m[0]=roll.m[5]=std::cos(shakeRoll); roll.m[1]=std::sin(shakeRoll); roll.m[4]=-std::sin(shakeRoll);
    const Mat4 view = roll*lookAt(eye, cameraForward);
    renderFov=encounter.presenting() && scareConfig.fovPunch ?
        encounter.visualFov() : fieldOfView;
    const Mat4 projection = perspective(renderFov * PI / 180, float(width) / height, .06f, SCENE_VIEW_DISTANCE);
    const Mat4 vp = projection * view;

    struct NearLight {
        float distance;
        Vec3 position;
        float power;
        Vec3 color;
        int slot = -1;
    };
    std::vector<NearLight> lights;
    for (const auto& [coord, chunk] : world.chunks()) {
        for (const auto& lamp : chunk.lamps) {
            const Vec3 position{
                float(coord.x * CHUNK_SIZE - originX) + lamp.x,
                lamp.y,
                float(coord.z * CHUNK_SIZE - originZ) + lamp.z
            };
            const float distance = dot(position - eye, position - eye);
            if (distance < 30 * 30 && lamp.state != LampState::Off) {
                lights.push_back({distance, position, lamp.power*lampEnvelope(lamp.state,lamp.phase,float(elapsed)), {lamp.r, lamp.g, lamp.b}});
            }
        }
    }
    const auto selectedEnd = lights.begin() + std::min<std::size_t>(24, lights.size());
    std::partial_sort(lights.begin(), selectedEnd, lights.end(), [](const NearLight& a, const NearLight& b) {
        return a.distance < b.distance;
    });
    if (lights.size() > 24) {
        lights.resize(24);
    }

    std::sort(lights.begin(),lights.end(),[](const NearLight& a,const NearLight& b){
        if(a.position.x!=b.position.x)return a.position.x<b.position.x;
        if(a.position.z!=b.position.z)return a.position.z<b.position.z;
        return a.position.y<b.position.y;
    });
    shadows.assignSlots(lights,originX,originZ);
    const Vec3 right=normalize(cross(cameraForward,{0,1,0}));
    Vec3 flashPosition=eye+right*.16f+Vec3{0,-.10f,0};
    // Keep the lamp on the camera side of a wall when hugging it.
    if(world.blocked(originX+flashPosition.x,originZ+flashPosition.z,.05))flashPosition=eye;
    const Mat4 flashVP=perspective(70.f*PI/180,1,.05f,35.f)*lookAt(flashPosition,cameraForward);
    auto drawShadowGeometry=[&](const Mat4& lightVP,bool actor) {
        shadow.set("uLightVP",lightVP);
        for(const auto& [coord,mesh]:gpu) {
            auto found=world.chunks().find(coord);if(found==world.chunks().end())continue;
            float x=float(coord.x*CHUNK_SIZE-originX),z=float(coord.z*CHUNK_SIZE-originZ);
            float h=found->second.maxHeight;
            if(!visibleBox(lightVP,{x+float(CHUNK_SIZE*.5),h*.5f,z+float(CHUNK_SIZE*.5)},
                {float(CHUNK_SIZE*.5+.2),h*.5f+.2f,float(CHUNK_SIZE*.5+.2)}))continue;
            shadow.set("uModel",translate(x,0,z));glBindVertexArray(mesh.vao);
            drawSections(mesh,lightVP,{x,0,z},{},false);
        }
        shadow.set("uModel",Mat4::identity());
        if(doorGpu.count){glBindVertexArray(doorGpu.vao);glDrawArrays(GL_TRIANGLES,0,doorGpu.count);}
        if(actor && partyGpu.count) {
            shadow.set("uModel",Mat4::identity());glBindVertexArray(partyGpu.vao);
            glDrawArrays(GL_TRIANGLES,0,partyGpu.count);
        }
        if(actor&&threat.active()&&threatGpu.count&&!encounter.presenting()) {
            Mat4 rotation=Mat4::identity();rotation.m[0]=rotation.m[10]=std::cos(threatBodyYaw);
            rotation.m[2]=std::sin(threatBodyYaw);rotation.m[8]=-std::sin(threatBodyYaw);
            shadow.set("uModel",translate(float(threat.x()-originX),0,float(threat.z()-originZ))*rotation);
            glBindVertexArray(threatGpu.vao);glDrawArrays(GL_TRIANGLES,0,threatGpu.count);
        }
    };
    shadow.use();glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(1.5f,2.f);
    if(flashlight&&!hiding&&!encounter.presenting()) {
        shadows.begin();drawShadowGeometry(flashVP,true);
    }
    int animatedShadowBudget=2;
    for(size_t i=0;i<lights.size();++i) {
        auto& cached=shadows.cache[lights[i].slot];const Vec3 pos=lights[i].position;
        std::uint64_t shadowRevision=1469598103934665603ULL;
        for(const auto& [coord,chunk]:world.chunks()) {
            const float x=float(coord.x*CHUNK_SIZE-originX),z=float(coord.z*CHUNK_SIZE-originZ);
            const float dx=pos.x-std::clamp(pos.x,x,x+float(CHUNK_SIZE));
            const float dz=pos.z-std::clamp(pos.z,z,z+float(CHUNK_SIZE));
            if(dx*dx+dz*dz>100)continue;
            shadowRevision=(shadowRevision^chunk.revision)*1099511628211ULL;
            for(const auto& door:chunk.doors) {
                const double dx=door.x-originX-pos.x,dz=door.z-originZ-pos.z;
                if(dx*dx+dz*dz>144)continue;
                shadowRevision=(shadowRevision^door.id)*1099511628211ULL;
                shadowRevision=(shadowRevision^std::uint64_t(door.swing>0?1:2))*1099511628211ULL;
                shadowRevision=(shadowRevision^std::uint64_t(std::lround(door.openness*8)))*1099511628211ULL;
            }
        }
        if(!cached.valid||cached.revision!=shadowRevision) {
            // Keep valid maps while coalescing animation updates. New lights
            // must initialize immediately; stationary endpoints catch up.
            if(cached.valid) {
                if(animatedShadowBudget<=0)continue;
                --animatedShadowBudget;
            }
            for(int face=0;face<6;++face){shadows.begin(lights[i].slot*6+face);drawShadowGeometry(lampFace(pos,face),false);}
            cached={pos,shadowRevision,true,originX+pos.x,originZ+pos.z};++lampShadowUpdates;
        }
    }
    glDisable(GL_POLYGON_OFFSET_FILL);glEnable(GL_CULL_FACE);glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER,target.fbo);glViewport(0,0,width,height);
    setupMs = (glfwGetTime() - setupStart) * 1000;
    const double sceneStart = glfwGetTime();
    // First pass: textured room geometry and nearby point/flashlight lighting.
    scene.use();
    scene.set("uView", view);
    scene.set("uProjection", projection);
    scene.set("uCamera", eye);
    scene.set("uWorldPhase",Vec3{float(std::fmod(std::fmod(originX,180.0)+180.0,180.0)),0,
                              float(std::fmod(std::fmod(originZ,180.0)+180.0,180.0))});
    scene.set("uForward", cameraForward);
    scene.set("uFlashPosition",flashPosition);scene.set("uFlashVP",flashVP);
    scene.set("uFlashShadows",int(flashlight&&!hiding&&!encounter.presenting()));
    scene.set("uTime", float(elapsed));
    scene.set("uFlashlight", int(flashlight && !hiding));
    scene.set("uExposure", settings.brightness);
    scene.set("uScareDarken",encounter.presenting() && scareConfig.darken ? 1.f : 0.f);
    scene.set("uScareGlow",encounter.presenting() && scareConfig.glow ? 1.f : 0.f);
    scene.set("uClues", int(clues));
    scene.set("uFog", 1);
    scene.set("uRenderDistance", SCENE_VIEW_DISTANCE);
    // Match the shader's fixed light capacity and clear unused slots so
    // lighting from the previous frame cannot leak into this one.
    std::array<float, 72> lightPositions{}, lightColors{};
    std::array<float, 24> lightPowers{};
    for (size_t i = 0; i < lights.size(); ++i) {
        const auto slot=lights[i].slot;
        lightPositions[slot * 3] = lights[i].position.x;
        lightPositions[slot * 3 + 1] = lights[i].position.y;
        lightPositions[slot * 3 + 2] = lights[i].position.z;
        lightColors[slot * 3] = lights[i].color.x;
        lightColors[slot * 3 + 1] = lights[i].color.y;
        lightColors[slot * 3 + 2] = lights[i].color.z;
        lightPowers[slot] = lights[i].power;
    }
    glUniform3fv(scene.location("uLights[0]"), 24, lightPositions.data());
    glUniform1fv(scene.location("uLightPower[0]"), 24, lightPowers.data());
    glUniform3fv(scene.location("uLightColor[0]"), 24, lightColors.data());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures.wallpaper);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures.carpet);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textures.ceiling);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, textures.wallpaperNormal);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, textures.carpetNormal);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, textures.ceilingNormal);
    glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, textures.hardFloor);
    glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, textures.hardFloorNormal);

    glActiveTexture(GL_TEXTURE8);glBindTexture(GL_TEXTURE_2D,shadows.flash);
    glActiveTexture(GL_TEXTURE9);glBindTexture(GL_TEXTURE_2D_ARRAY,shadows.lamps);
    glActiveTexture(GL_TEXTURE10);glBindTexture(GL_TEXTURE_2D,textures.creatureColor);
    glActiveTexture(GL_TEXTURE11);glBindTexture(GL_TEXTURE_2D,textures.creatureNormal);
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    drawn = 0;
    drawnTriangles = 0;
    for (const auto& [coord, mesh] : gpu) {
        if (!world.chunks().count(coord)) continue; // Prefetched meshes stay invisible.
        const float x = float(coord.x * CHUNK_SIZE - originX);
        const float z = float(coord.z * CHUNK_SIZE - originZ);
        const float ceilingHeight = world.chunks().at(coord).maxHeight;
        const Vec3 center{x + float(CHUNK_SIZE * .5), ceilingHeight * .5f,
                          z + float(CHUNK_SIZE * .5)};
        const Vec3 half{float(CHUNK_SIZE * .5 + .2), ceilingHeight * .5f + .2f,
                        float(CHUNK_SIZE * .5 + .2)};
        if (!visibleBox(vp, center, half)) {
            continue;
        }
        scene.set("uModel", translate(x, 0, z));
        glBindVertexArray(mesh.vao);
        drawnTriangles += drawSections(mesh,vp,{x,0,z},eye,true);
        ++drawn;
    }
    uploadVertices(mouseGpu,mouse.vertices(originX,originZ),GL_DYNAMIC_DRAW);
    if(mouseGpu.count) {
        scene.set("uModel",translate(0,0,0));glBindVertexArray(mouseGpu.vao);
        glDrawArrays(GL_TRIANGLES,0,mouseGpu.count);drawnTriangles+=mouseGpu.count/3;
    }
    uploadVertices(verityGpu,world.verityVertices(originX,originZ,px,pz,float(walk)),GL_DYNAMIC_DRAW);
    if(verityGpu.count) {
        scene.set("uModel",translate(0,0,0));glBindVertexArray(verityGpu.vao);
        glDrawArrays(GL_TRIANGLES,0,verityGpu.count);drawnTriangles+=verityGpu.count/3;
    }
    uploadVertices(keyGpu,world.keyVertices(originX,originZ),GL_DYNAMIC_DRAW);
    if(keyGpu.count) {
        scene.set("uModel",translate(0,0,0));glBindVertexArray(keyGpu.vao);
        glDrawArrays(GL_TRIANGLES,0,keyGpu.count);drawnTriangles+=keyGpu.count/3;
    }
    if(doorGpu.count) {
        scene.set("uModel",translate(0,0,0));
        glBindVertexArray(doorGpu.vao);
        glDrawArrays(GL_TRIANGLES,0,doorGpu.count);
        drawnTriangles+=doorGpu.count/3;
    }
    if(partyGpu.count && world.isFun()) {
        scene.set("uModel",Mat4::identity());glBindVertexArray(partyGpu.vao);
        glDrawArrays(GL_TRIANGLES,0,partyGpu.count);drawnTriangles+=partyGpu.count/3;
    }
    if(darknessGpu.count && world.isParking()) {
        if(encounter.presenting()) {
            uploadVertices(darknessGpu,Darkness::scareVertices(eye,cameraForward,encounter.lunge()),GL_DYNAMIC_DRAW);
            glDisable(GL_DEPTH_TEST);
        }
        scene.set("uModel",Mat4::identity());glBindVertexArray(darknessGpu.vao);
        glDrawArrays(GL_TRIANGLES,0,darknessGpu.count);drawnTriangles+=darknessGpu.count/3;
        glEnable(GL_DEPTH_TEST);
    }
    if(threat.active() && threatGpu.count) {
        float tx=float(threat.x()-originX),tz=float(threat.z()-originZ),ty=0,yaw=threatBodyYaw;
        Vec3 captureDirection=cameraForward;
        if(encounter.scaring()) {
            // A short 3D close-up uses the same articulated mesh. Freeze the
            // player and keep the face visible even at a wall or low camera angle.
            const float lunge=scareConfig.lunge?encounter.lunge():0;
            const float distance=1.65f+(scareConfig.faceDistance-1.65f)*lunge;
            const Vec3 right=normalize(cross(cameraForward,{0,1,0}));
            const float tremor=scareConfig.shake?std::sin(encounter.elapsed()*97.f)*.012f*(1-encounter.progress()):0;
            // Hold the attack in the creature's direction while the
            // camera turns toward its head, then finish directly in front.
            captureDirection=scareConfig.cameraTurn?scareApproach:cameraForward;
            const Vec3 target=stableEye+captureDirection*distance+right*tremor;
            tx=target.x;tz=target.z;
            ty=target.y-THREAT_EYE_HEIGHT+.03f;
            yaw=std::atan2(-captureDirection.x,captureDirection.z);
            glClear(GL_DEPTH_BUFFER_BIT);
            glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
            scene.set("uFlashlight",1);
        }
        if(encounter.scaring() || visibleBox(vp,{tx,ty+1.5f,tz},{2.7f,1.6f,2.7f})) {
            Mat4 rotation=Mat4::identity();
            rotation.m[0]=rotation.m[10]=std::cos(yaw);
            rotation.m[2]=std::sin(yaw); rotation.m[8]=-std::sin(yaw);
            Mat4 actor=translate(tx,ty,tz)*rotation;
            if(encounter.presenting()) {
                // Pivot at the rig's eye anchor so the face looks directly
                // back at the camera, including when the player is crouched.
                const float tilt=-std::asin(std::clamp(captureDirection.y,-1.f,1.f));
                Mat4 pitch=Mat4::identity(),scale=Mat4::identity();
                pitch.m[5]=pitch.m[10]=std::cos(tilt);
                pitch.m[6]=std::sin(tilt); pitch.m[9]=-std::sin(tilt);
                const float size=scareConfig.lunge?1+(scareConfig.closeupScale-1)*encounter.lunge():1;
                scale.m[0]=scale.m[5]=scale.m[10]=size;
                actor=translate(tx,ty+THREAT_EYE_HEIGHT,tz)*rotation*pitch*scale*translate(0,-THREAT_EYE_HEIGHT,0);
            }
            scene.set("uModel",actor);
            glBindVertexArray(threatGpu.vao);
            glDrawArrays(GL_TRIANGLES,0,threatGpu.count);
            drawnTriangles+=threatGpu.count/3;
        }
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    sceneMs = (glfwGetTime() - sceneStart) * 1000;
    const double lensStart = glfwGetTime();
    // Half-resolution contact AO, followed by depth-aware upsampling.
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glDisable(GL_CULL_FACE);glDisable(GL_DEPTH_TEST);
    if(settings.ambientOcclusion>0&&!encounter.presenting()&&!wireframe) {
        glBindFramebuffer(GL_FRAMEBUFFER,ambientTarget.fbo);
        glViewport(0,0,ambientTarget.width,ambientTarget.height);
        ao.use();ao.set("uProjection",projection);ao.set("uSize",Vec3{float(width),float(height),0});
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,target.depth);
        glBindVertexArray(emptyVao);glDrawArrays(GL_TRIANGLES,0,3);
    }
    glViewport(0,0,width,height);
    // Lens treatment on the default framebuffer.
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    lens.use();
    lens.set("uTime", float(elapsed));
    lens.set("uBlurAmount", settings.blur);
    lens.set("uAOAmount",encounter.presenting()||wireframe?0.f:settings.ambientOcclusion);
    lens.set("uProjection",projection);
    glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,ambientTarget.texture);
    glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,target.depth);
    lens.set("uFear",encounter.alive() && threat.state()==ThreatState::Chase ? .28f : 0.f);
    lens.set("uSize", Vec3{float(width), float(height), 0});
    const bool scare=encounter.presenting();
    lens.set("uScareTime",encounter.elapsed());
    lens.set("uChromatic",scare && scareConfig.chromatic ? scareConfig.chromaticAmount : 0.f);
    lens.set("uStatic",scare && scareConfig.noise ? scareConfig.staticAmount : 0.f);
    lens.set("uVignette",scare && scareConfig.vignette ? scareConfig.vignetteAmount : 0.f);
    lens.set("uRed",scare && scareConfig.redTint ? std::max(0.f,(encounter.progress()-.55f)/.45f)*scareConfig.redAmount : 0.f);
    lens.set("uGlow",scare && scareConfig.glow ? 1.f : 0.f);
    lens.set("uFlash",encounter.flashForDraw());
    lens.set("uBlack",encounter.black()?1.f:0.f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, target.color);
    glBindVertexArray(emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    lensMs = (glfwGetTime() - lensStart) * 1000;
    uiStart = glfwGetTime();
    // Overlays use framebuffer pixels, so high-DPI rendering stays crisp.
    lightCount = lights.size();
    return true;
}

} // namespace br::app
