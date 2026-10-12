// Educational prototype. See docs/AI_USE.md for an accurate assistance disclosure.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "world.h"
#include "common/math.h"
#include "common/Shader.h"
#include "common/graphics.h"
#include "common/textures.h"
#include "common/ui.h"
#include "common/controls_guide.h"
#include "common/audio.h"
#include "common/movement.h"
#include "common/gameplay.h"
#include "common/player_state.h"
#include "common/settings.h"
#include "common/music.h"
#include "common/threat.h"
#include "common/mouse.h"
#include "common/threat_model.h"
#include "common/encounter.h"

namespace fs = std::filesystem;
using namespace br;

constexpr int MIN_WINDOW_WIDTH = 960;
constexpr int MIN_WINDOW_HEIGHT = 640;
static std::ofstream logFile;

static void log(const std::string& message) {
    std::cout << message << '\n';
    if (logFile) {
        logFile << message << '\n';
        logFile.flush();
    }
}

static void glfwError(int code, const char* message) {
    log("GLFW " + std::to_string(code) + ": " + message);
}

struct Options {
    uint64_t seed = 12071998;
    int frames = 0, width = 1280, height = 800;
    bool hidden = false, demo = false, tour = false;
    bool noFlashlight = false, map = false, metrics = false;
    bool wireframe = false, autoWalk = false, autoExit = false, autoContinue = false;
    bool autoCrouch = false, autoStand = false, autoSprint = false, cameraView = false;
    bool autoStop = false, autoReleaseSprint = false;
    bool unboundedQueue = false, settingsMenu = false, autoSettings = false, musicMenuCheck = false;
    bool autoDoor = false, autoCloseDoor = false, menuCheck = false, autoPause = false;
    bool noThreat=false, threatPreview=false, forcedThreat=false;
    bool autoKey=false, autoHide=false, hideFixture=false;
    bool mouseFixture=false, mousePreview=false, guideCheck=false, guideClose=false, guideClick=false;
    bool guideBack=false, guideSkip=false;
    bool autoCatch=false, autoRestart=false, autoRestartClick=false, autoScareEscape=false, autoJumpscare=false;
    double threatX=8, threatZ=3.75;
    float threatYaw=-90;
    float threatPreviewTime=0, threatPreviewSpeed=0, threatPreviewHeadYaw=0, threatPreviewHeadPitch=0;
    std::string settingsFile, musicFile;
    int doorCloseFrame=110;
    double spawnX = CELL*.5, spawnZ = CELL*.5;
    float yaw = -90, pitch = -3;
    std::string capture, audioPreview, profile;
    std::string jumpscareConfigFile="jumpscare.cfg";
};

static Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing argument for " + argument);
            }
            return argv[++i];
        };

        if (argument == "--seed") {
            options.seed = std::stoull(next());
        } else if (argument == "--frames") {
            options.frames = std::stoi(next());
        } else if (argument == "--width") {
            options.width = std::stoi(next());
        } else if (argument == "--height") {
            options.height = std::stoi(next());
        } else if (argument == "--x") {
            options.spawnX = std::stod(next());
        } else if (argument == "--z") {
            options.spawnZ = std::stod(next());
        } else if (argument == "--yaw") {
            options.yaw = std::stof(next());
        } else if (argument == "--pitch") {
            options.pitch = std::stof(next());
        } else if (argument == "--capture") {
            options.capture = next();
        } else if (argument == "--bounded-queue") {
            options.unboundedQueue = false;
        } else if (argument == "--unbounded-queue") {
            options.unboundedQueue = true;
        } else if (argument == "--jumpscare-config") {
            options.jumpscareConfigFile=next();
        } else if (argument == "--profile") {
            options.profile = next();
        } else if (argument == "--hidden") {
            options.hidden = true;
        } else if (argument == "--demo") {
            options.demo = true;
        } else if (argument == "--tour") {
            options.tour = true;
        } else if (argument == "--no-flashlight") {
            options.noFlashlight = true;
        } else if (argument == "--map") {
            options.map = true;
        } else if (argument == "--metrics") {
            options.metrics = true;
        } else if (argument == "--wireframe") {
            options.wireframe = true;
        } else if (argument == "--auto-walk") {
            options.autoWalk = true;
        } else if (argument == "--auto-exit") {
            options.autoExit = true;
        } else if (argument == "--auto-continue") {
            options.autoContinue = true;
        } else if (argument == "--auto-crouch") {
            options.autoCrouch = true;
        } else if (argument == "--auto-stand") {
            options.autoStand = true;
        } else if (argument == "--auto-sprint") {
            options.autoSprint = true;
        } else if (argument == "--auto-stop") {
            options.autoStop = true;
        } else if (argument == "--auto-release-sprint") {
            options.autoReleaseSprint = true;
        } else if (argument == "--camera-view") {
            options.cameraView = true;
        } else if (argument == "--settings") {
            options.settingsMenu = true;
        } else if (argument == "--auto-settings") {
            options.settingsMenu = options.autoSettings = true;
        } else if (argument == "--settings-file") {
            options.settingsFile = next();
        } else if (argument == "--music-file") {
            options.musicFile = next();
        } else if (argument == "--music-menu-check") {
            options.settingsMenu = options.musicMenuCheck = true;
        } else if (argument == "--door-close-frame") {
            options.doorCloseFrame = std::stoi(next());
        } else if (argument == "--menu-check") {
            options.menuCheck = true;
        } else if (argument == "--auto-pause") {
            options.autoPause = true;
        } else if (argument == "--auto-door") {
            options.autoDoor = true;
        } else if (argument == "--auto-close-door") {
            options.autoDoor = options.autoCloseDoor = true;
        } else if (argument == "--no-threat") { options.noThreat=true;
        } else if (argument == "--threat-preview") { options.threatPreview=true; options.forcedThreat=true;
        } else if (argument == "--threat-x") { options.threatX=std::stod(next()); options.forcedThreat=true;
        } else if (argument == "--threat-z") { options.threatZ=std::stod(next()); options.forcedThreat=true;
        } else if (argument == "--threat-yaw") { options.threatYaw=std::stof(next()); options.forcedThreat=true;
        } else if (argument == "--threat-time") { options.threatPreviewTime=std::stof(next());
        } else if (argument == "--threat-speed") { options.threatPreviewSpeed=std::stof(next());
        } else if (argument == "--threat-head-yaw") { options.threatPreviewHeadYaw=std::stof(next());
        } else if (argument == "--threat-head-pitch") { options.threatPreviewHeadPitch=std::stof(next());
        } else if (argument == "--auto-catch") { options.autoCatch=true;
        } else if (argument == "--auto-restart") { options.autoRestart=true;
        } else if (argument == "--auto-restart-click") { options.autoRestartClick=true;
        } else if (argument == "--auto-scare-escape") { options.autoScareEscape=true;
        } else if (argument == "--auto-key") { options.autoKey=true;
        } else if (argument == "--mouse-fixture") { options.mouseFixture=true;
        } else if (argument == "--mouse-preview") { options.mousePreview=true;
        } else if (argument == "--guide-check") { options.guideCheck=true;
        } else if (argument == "--guide-close") { options.guideClose=true;
        } else if (argument == "--guide-click") { options.guideClick=true;
        } else if (argument == "--guide-back") { options.guideBack=true;
        } else if (argument == "--guide-skip") { options.guideSkip=true;
        } else if (argument == "--auto-hide") { options.autoHide=true;
        } else if (argument == "--hide-fixture") { options.hideFixture=true;
        } else if (argument == "--auto-jumpscare") { options.autoJumpscare=true;
        } else if (argument == "--audio-preview") {
            options.audioPreview = next();
        } else if (argument == "--help") {
            std::cout << "Backrooms: --seed N --frames N --capture image.bmp --hidden "
                         "--demo --tour --x X --z Z --yaw DEGREES --width N --height N\n"
                         "Verification: --no-flashlight --map --metrics --wireframe --auto-walk\n"
                         "--auto-exit simulates E once; --auto-continue simulates Enter afterward.\n"
                         "--auto-crouch --auto-stand --auto-sprint test movement modes.\n"
                         "--auto-stop or --auto-release-sprint releases scripted input halfway.\n"
                         "--camera-view hides the normal HUD. --pitch DEGREES sets the camera tilt.\n"
                         "--audio-preview file.wav exports sounds without playback or a GL context.\n"
                         "--profile frames.csv records per-phase frame timings.\n"
                         "--unbounded-queue disables normal two-frame GPU pacing for comparisons.\n"
                         "--auto-walk: forward movement with fixed 1/60-second simulation "
                         "steps; use --demo to begin unpaused. FPS uses real elapsed time.\n"
                         "--settings opens settings; --auto-settings verifies changing brightness.\n"
                         "--settings-file path uses a custom settings file (hidden runs otherwise ignore saved settings).\n"
                         "--auto-door / --auto-close-door verify a nearby door interaction.\n"
                         "--menu-check exercises settings and resume; --auto-pause pauses on frame 2.\n"
                         "--music-file path selects an optional local MP3/WAV/M4A song.\n"
                         "--door-close-frame N changes the scripted closing time (default 110).\n"
                         "Threat verification: --threat-x X --threat-z Z --threat-yaw DEGREES\n"
                         "--threat-preview freezes the model/AI. --no-threat isolates room checks.\n"
                         "--threat-time SECONDS --threat-speed MPS select an animation preview pose.\n"
                         "--auto-catch [--auto-restart | --auto-restart-click] checks the defeat flow.\n"
                         "--auto-scare-escape tests Escape during capture and game over.\n"
                         "--jumpscare-config PATH loads optional jumpscare timing/camera settings.\n"
                         "Minimum window size: 960 x 640.\n";
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown option: " + argument);
        }
    }

    if (options.width < MIN_WINDOW_WIDTH || options.width > 7680 ||
        options.height < MIN_WINDOW_HEIGHT || options.height > 4320) {
        throw std::runtime_error("Window size must be between 960x640 and 7680x4320.");
    }
    if (options.frames < 0 || options.doorCloseFrame < 0 || !std::isfinite(options.spawnX) ||
        !std::isfinite(options.spawnZ) || !std::isfinite(options.yaw) || !std::isfinite(options.pitch) ||
        !std::isfinite(options.threatX) || !std::isfinite(options.threatZ) || !std::isfinite(options.threatYaw) ||
        !std::isfinite(options.threatPreviewTime) || options.threatPreviewTime<0 || options.threatPreviewTime>10000 ||
        !std::isfinite(options.threatPreviewHeadYaw) || std::abs(options.threatPreviewHeadYaw)>90 ||
        !std::isfinite(options.threatPreviewHeadPitch) || std::abs(options.threatPreviewHeadPitch)>30 ||
        !std::isfinite(options.threatPreviewSpeed) || options.threatPreviewSpeed<0 || options.threatPreviewSpeed>10 ||
        std::abs(options.threatX)>1e12 || std::abs(options.threatZ)>1e12 ||
        std::abs(options.pitch) > 85 ||
        std::abs(options.spawnX) > 1e12 || std::abs(options.spawnZ) > 1e12) {
        throw std::runtime_error("Invalid frame count or spawn coordinate.");
    }
    if (options.hidden && options.frames == 0) {
        options.frames = 60;
    }
    return options;
}

// A GPU mesh is rebuilt only when its corresponding streamed chunk changes.
struct GpuChunk {
    GLuint vao = 0, vbo = 0;
    GLsizei count = 0;
    uint64_t revision = 0;
};

static void release(GpuChunk& chunk) {
    if (chunk.vbo) {
        glDeleteBuffers(1, &chunk.vbo);
    }
    if (chunk.vao) {
        glDeleteVertexArrays(1, &chunk.vao);
    }
    chunk = {};
}

static void uploadVertices(GpuChunk& chunk, const std::vector<Vertex>& vertices,
                           GLenum usage = GL_STATIC_DRAW) {
    if (!chunk.vao) {
        glGenVertexArrays(1, &chunk.vao);
        glGenBuffers(1, &chunk.vbo);
    }
    chunk.count = static_cast<GLsizei>(vertices.size());

    glBindVertexArray(chunk.vao);
    glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), usage);
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(i);
    }
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, x)));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, nx)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, u)));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, material)));
    glBindVertexArray(0);
}

static void upload(GpuChunk& chunk, const Chunk& data) {
    uploadVertices(chunk, data.vertices);
    chunk.revision = data.revision;
}

// Keep the CPU at most two completed submissions ahead of the GPU. This
// prevents an uncapped render loop from queuing hundreds of frames and then
// blocking for seconds inside a later buffer clear, while reducing input lag.
struct FrameQueue {
    std::array<GLsync, 2> fences{};
    std::size_t next = 0;
    void wait() {
        if (!fences[next]) return;
        GLenum status;
        do {
            status = glClientWaitSync(fences[next], GL_SYNC_FLUSH_COMMANDS_BIT, 50000000);
            if (status == GL_WAIT_FAILED) throw std::runtime_error("GPU frame synchronization failed.");
        } while (status == GL_TIMEOUT_EXPIRED);
        glDeleteSync(fences[next]);
        fences[next] = nullptr;
    }
    void submit() {
        fences[next] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        if (!fences[next]) throw std::runtime_error("Cannot create GPU frame fence.");
        next = (next + 1) % fences.size();
    }
    void destroy() {
        for (auto& fence : fences) {
            if (fence) glDeleteSync(fence);
            fence = nullptr;
        }
    }
};

// Render the room into a texture, then apply the camera treatment as a second pass.
struct SceneTarget {
    GLuint fbo = 0, color = 0, depth = 0;
    int width = 0, height = 0;

    void resize(int w, int h) {
        if (width == w && height == h) {
            return;
        }
        destroy();
        width = w;
        height = h;

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glGenTextures(1, &color);
        glBindTexture(GL_TEXTURE_2D, color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, color, 0);

        glGenTextures(1, &depth);
        glBindTexture(GL_TEXTURE_2D,depth);
        glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,w,h,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depth,0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("Offscreen framebuffer is incomplete.");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void destroy() {
        if (depth) {
            glDeleteTextures(1, &depth);
        }
        if (color) {
            glDeleteTextures(1, &color);
        }
        if (fbo) {
            glDeleteFramebuffers(1, &fbo);
        }
        depth = color = fbo = 0;
        width = height = 0;
    }
};

static void screenshot(const fs::path& path, int width, int height) {
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }
    const int rowSize = (width * 3 + 3) & ~3;
    std::vector<unsigned char> pixels(size_t(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // BMP rows and OpenGL's origin both start at the bottom of the image.
    std::array<unsigned char, 54> header{};
    header[0] = 'B';
    header[1] = 'M';
    auto put = [&](int offset, uint32_t value) {
        for (int i = 0; i < 4; ++i) {
            header[offset + i] = static_cast<unsigned char>(value >> (8 * i));
        }
    };
    put(2, 54 + rowSize * height);
    put(10, 54);
    put(14, 40);
    put(18, width);
    put(22, height);
    header[26] = 1;
    header[28] = 24;
    put(34, rowSize * height);

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot save screenshot: " + path.string());
    }
    file.write(reinterpret_cast<char*>(header.data()), header.size());
    std::vector<unsigned char> scan(rowSize);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const size_t pixel = (size_t(y) * width + x) * 3;
            scan[x * 3] = pixels[pixel + 2];
            scan[x * 3 + 1] = pixels[pixel + 1];
            scan[x * 3 + 2] = pixels[pixel];
        }
        file.write(reinterpret_cast<char*>(scan.data()), rowSize);
    }
    if (!file) {
        throw std::runtime_error("Failed to write screenshot.");
    }
    log("Screenshot saved: " + path.string());
}

struct Input {
    GLFWwindow* window = nullptr;
    std::array<bool, GLFW_KEY_LAST + 1> pressed{}, pendingPressed{};
    bool paused = true, firstMouse = true;
    double lastX = 0, lastY = 0;
    float yaw = -90, pitch = -3, sensitivity = .095f;
    SmoothLook look;
    bool clicked = false, pendingClick = false;

    void setLook(float y, float p) { yaw = y; pitch = p; look.reset(y, p); }
    void smooth(float dt) { if (!paused) { look.update(dt); yaw=look.yaw; pitch=look.pitch; } }
    void poll() {
        // A press followed by a release within one frame must still toggle the
        // action. The callback latches press events until the next frame consumes them.
        pressed = pendingPressed;
        pendingPressed.fill(false);
        clicked = pendingClick; pendingClick = false;
    }

    bool hit(int key) const { return pressed[key]; }
    bool down(int key) const { return glfwGetKey(window, key) == GLFW_PRESS; }

    static void key(GLFWwindow* window, int key, int, int action, int) {
        auto* self = static_cast<Input*>(glfwGetWindowUserPointer(window));
        if (self && key >= 0 && key <= GLFW_KEY_LAST && action == GLFW_PRESS) {
            self->pendingPressed[key] = true;
        }
    }

    static void button(GLFWwindow* window, int button, int action, int) {
        auto* self = static_cast<Input*>(glfwGetWindowUserPointer(window));
        if (self && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) self->pendingClick = true;
    }
    void pause(bool value) {
        paused = value;
        firstMouse = true;
        look.reset(yaw, pitch);
        glfwSetInputMode(window, GLFW_CURSOR,
                         value ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }

    Vec3 forward() const {
        const float y = yaw * PI / 180, p = pitch * PI / 180;
        return normalize({std::cos(y) * std::cos(p), std::sin(p),
                          std::sin(y) * std::cos(p)});
    }

    static void mouse(GLFWwindow* window, double x, double y) {
        auto* self = static_cast<Input*>(glfwGetWindowUserPointer(window));
        if (!self || self->paused) {
            return;
        }
        if (self->firstMouse) {
            self->lastX = x;
            self->lastY = y;
            self->firstMouse = false;
        }
        self->look.add(float(x - self->lastX), float(y - self->lastY), self->sensitivity);
        self->lastX = x;
        self->lastY = y;
    }
};

static void minimap(UI& ui, const World& world, double px, double pz,
                    Vec3 forward, int width) {
    constexpr float size = 228, scale = 2.3f;
    const float left = float(width) - size - 26, top = 80;
    const float cx = left + size * .5f, cy = top + size * .5f;
    ui.rect(left, top, size, size, {.035f, .04f, .032f, .91f});
    ui.rect(left, top, size, 2, {.7f, .65f, .36f, .7f});

    for (const auto& [coord, chunk] : world.chunks()) {
        const double ox = coord.x * CHUNK_SIZE, oz = coord.z * CHUNK_SIZE;
        for (const auto& bounds : chunk.obstacles) {
            float x = cx + float(ox + bounds.minX - px) * scale;
            float y = cy + float(oz + bounds.minZ - pz) * scale;
            float x2 = cx + float(ox + bounds.maxX - px) * scale;
            float y2 = cy + float(oz + bounds.maxZ - pz) * scale;
            x = std::clamp(x, left + 5, left + size - 5);
            x2 = std::clamp(x2, left + 5, left + size - 5);
            y = std::clamp(y, top + 5, top + size - 5);
            y2 = std::clamp(y2, top + 5, top + size - 5);
            if (x2 > x && y2 > y) {
                ui.rect(x, y, std::max(1.f, x2 - x), std::max(1.f, y2 - y),
                        {.51f, .49f, .33f, .8f});
            }
        }
    }
    const auto door = world.exitLocation();
    const float exitX = cx + float(door.x - px) * scale;
    const float exitY = cy + float(door.z - pz) * scale;
    if (exitX > left + 8 && exitX < left + size - 8 &&
        exitY > top + 8 && exitY < top + size - 8) {
        ui.rect(exitX - 4, exitY - 4, 8, 8, {.3f, 1.f, .65f, 1});
        ui.text(std::min(exitX + 7, left + size - 36), exitY - 3, "EXIT", 1, {.5f, 1, .75f, 1});
    }
    if(!world.keyCollected()) {
        const auto key=world.keyLocation();
        const float kx=std::clamp(cx+float(key.x-px)*scale,left+10,left+size-30);
        const float ky=std::clamp(cy+float(key.z-pz)*scale,top+10,top+size-12);
        ui.rect(kx-3,ky-3,6,6,{1,.78f,.25f,1});ui.text(kx+7,ky-3,"KEY",1,{1,.78f,.25f,1});
    }
    for(const auto& [coord,chunk]:world.chunks()) for(const auto& spot:chunk.hidingSpots) {
        (void)coord;
        const float sx=cx+float(spot.approach.x-px)*scale,sy=cy+float(spot.approach.z-pz)*scale;
        if(sx>left+8 && sx<left+size-15 && sy>top+8 && sy<top+size-12) ui.text(sx,sy,"H",1,{.65f,.72f,.65f,1});
    }
    ui.rect(cx - 3, cy - 3, 6, 6, {.95f, .8f, .37f, 1});
    ui.line(cx, cy, cx + forward.x * 15, cy + forward.z * 15, 2, {1, .9f, .6f, 1});
    ui.text(left + 10, top + size + 10, "LOCAL MAP / LOADED AREA",
            1.25f, {.8f, .78f, .62f, 1});

}

static int run(const Options& options) {
    // Create a desktop OpenGL 3.3 core context and load its entry points.
    glfwSetErrorCallback(glfwError);
    if (!glfwInit()) {
        throw std::runtime_error("GLFW could not initialize. Check your graphics driver.");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, options.hidden ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 0);
    GLFWwindow* window = glfwCreateWindow(options.width, options.height,
        "BACKROOMS / Threshold - CGF Prototype v0.12", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Cannot create an OpenGL 3.3 core window.");
    }
    glfwSetWindowSizeLimits(window, MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT,
                            GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window);
        glfwTerminate();
        throw std::runtime_error("OpenGL loader failed.");
    }
    log(std::string("OpenGL: ") + reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    log(std::string("Renderer: ") + reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    Settings settings;
    const std::string settingsPath = options.settingsFile.empty() ? "settings.cfg" : options.settingsFile;
    if (!options.hidden || !options.settingsFile.empty()) settings.load(settingsPath);
    glfwSwapInterval(options.hidden ? 0 : int(settings.vsync));
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glClearColor(.065f, .060f, .035f, 1);

    Shader scene("vshader_backrooms.glsl", "fshader_backrooms.glsl");
    Shader lens("vshader_lens.glsl", "fshader_lens.glsl");
    UI ui;
    Textures textures = createTextures();
    SceneTarget target;
    ShadowMaps shadows; shadows.init();
    AmbientTarget ambientTarget;
    Shader shadow("vshader_shadow.glsl","fshader_shadow.glsl");
    Shader ao("vshader_lens.glsl","fshader_ao.glsl");
    std::uint64_t lampShadowUpdates=0;
    GLuint emptyVao = 0;
    glGenVertexArrays(1, &emptyVao);

    scene.use();
    scene.set("uWallpaper", 0);
    scene.set("uCarpet", 1);
    scene.set("uCeiling", 2);
    scene.set("uWallpaperNormal", 3);
    scene.set("uCarpetNormal", 4);
    scene.set("uCeilingNormal", 5);
    scene.set("uHardFloor", 6);
    scene.set("uHardFloorNormal", 7);
    scene.set("uCreatureColor",10);scene.set("uCreatureNormal",11);
    lens.use();
    lens.set("uScene", 0);
    lens.set("uAO",1);lens.set("uDepth",2);
    scene.use();scene.set("uFlashShadow",8);scene.set("uLampShadows",9);
    ao.use();ao.set("uDepth",0);

    World world(options.seed);
    double px = options.spawnX, pz = options.spawnZ;
    world.update(px, pz);
    if (world.blocked(px, pz, .23)) {
        throw std::runtime_error("Requested spawn intersects a wall.");
    }

    Threat threat(world.seed());
    Mouse mouse;mouse.reset(world.seed());int mouseSqueaks=0;
    JumpscareManager encounter(loadJumpscareConfig(options.jumpscareConfigFile));
    const auto& scareConfig=encounter.config();
    log("Jumpscare: duration "+std::to_string(scareConfig.duration)+" s, turn "+std::to_string(scareConfig.cameraTurnDuration)+" s, face "+std::to_string(scareConfig.faceDistance)+" m.");
    bool hiding=false;
    bool pendingKeyRoar=false;
    HidingSpot hidingSpot{};
    float roarCooldown=0,occlusionClock=0,threatTransmission=1;
    float scareYaw=0, scarePitch=0, scareTargetYaw=0, scareTargetPitch=0;
    Vec3 scareApproach{0,0,-1};
    if(options.forcedThreat && !options.noThreat) {
        if(world.blocked(options.threatX,options.threatZ,Threat::RADIUS))
            throw std::runtime_error("Threat fixture intersects a wall or unloaded chunk.");
        threat.resetAt(options.threatX,options.threatZ,options.threatYaw*PI/180);
    }
    Input input;
    input.window = window;
    input.setLook(options.yaw, options.pitch);
    input.sensitivity = settings.sensitivity;
    glfwSetWindowUserPointer(window, &input);
    glfwSetCursorPosCallback(window, Input::mouse);
    glfwSetKeyCallback(window, Input::key);
    glfwSetMouseButtonCallback(window, Input::button);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    input.pause(options.settingsMenu || !(options.demo || options.tour));

    std::map<ChunkCoord, GpuChunk> gpu;
    GpuChunk doorGpu, threatGpu, keyGpu, mouseGpu;
    // Aim the capture at the visible skull, which projects ahead of the root.
    auto headCenter=[](const std::vector<Vertex>& vertices) {
        Vec3 low{100,100,100},high{-100,-100,-100};bool found=false;
        for(const auto& v:vertices)if(v.material==THREAT_EYE_MATERIAL) {
            found=true;low={std::min(low.x,v.x),std::min(low.y,v.y),std::min(low.z,v.z)};
            high={std::max(high.x,v.x),std::max(high.y,v.y),std::max(high.z,v.z)};
        }
        return found?Vec3{(low.x+high.x)*.5f,low.y+(high.y-low.y)*.7f,(low.z+high.z)*.5f}:Vec3{0,THREAT_EYE_HEIGHT,0};
    };
    Vec3 threatHeadLocal{0,THREAT_EYE_HEIGHT,0};
    // Read and validate the imported animation cache during loading, so the
    // first encounter never has to open a file or allocate the clip bank.
    {
        const auto initialModel=threatModelVertices(0,0,false);
        uploadVertices(threatGpu,initialModel,GL_DYNAMIC_DRAW);
        threatHeadLocal=headCenter(initialModel);
        log("Loaded Aranea model: "+std::to_string(initialModel.size()/3)+" triangles.");
    }
    float threatAnimation=0, lastThreatMeshTime=-1;
    float threatStoop=0, lastThreatStoop=-1, threatNoticeCooldown=0;
    float threatGaitPhase=0, threatVisualSpeed=0, threatHeadYaw=0, threatHeadPitch=0;
    float threatBodyYaw=threat.yaw();
    ThreatFootContacts lastThreatContact{};
    int threatFootfalls=0;
    bool lastScareMesh=false, scriptedRestarted=false;
    int catches=0, restarts=0;
    bool doorVisualDirty=true;
    ChunkCoord doorOrigin{};
    std::size_t doorShift=0;
    BackgroundMusic music;
    const bool musicLoaded = music.load(options.musicFile.empty() ? fs::path("assets/audio/music/numbers-temporex.mp3") : fs::path(options.musicFile));
    if(musicLoaded) log("Background music loaded.");
    else if(!music.error().empty()) log("Background music unavailable: "+music.error());
    else log("No optional music file found; environmental sound remains available.");
    bool musicErrorReported=!music.error().empty();
    Audio audio;
    audio.setVolume(settings.volume);
    audio.reset(static_cast<uint32_t>(options.seed));
    bool muted = options.hidden, flashlight = !options.noFlashlight, clues = true;
    bool showMap = options.map, showHelp = false, diagnostics = options.metrics;
    int guidePage=0;
    bool guidePending=(!options.hidden && !options.tour) || options.guideCheck;
    bool wireframe = options.wireframe, shifting = true, entered = !input.paused;
    bool escaped = false, exitUsed = false, exitArmed = true;
    PlayerMotion motion;
    Stamina stamina;
    bool settingsOpen = options.settingsMenu;
    int menuSelection = 0, settingSelection = 0, doorsUsed = 0;
    float tension = 0, minimumStamina = 100;
    float bobOffset = 0;
    bool crouched = false, cameraView = options.cameraView;
    float eyeHeight = 1.65f;
    float fieldOfView = settings.fov;
    if (!muted) {
        audio.start();
        log(audio.available() ? "Audio output active." : "Audio output unavailable; use M to retry.");
    }

    const double runStart = glfwGetTime();
    double lastTime = runStart, elapsed = 0, walk = 0, travel = 0;
    double shiftClock = 0, fpsTime = 0, maxFrameMs = 0;
    int frame = 0, fpsFrames = 0;
    float fps = 0, noticeTime = 0;
    size_t maxChunks = 0, maxPrepared = 0;
    std::string notice;

    auto returnToStart = [&]() {
        px = pz = CELL*.5;
        input.setLook(-90, -3);
        world.update(px, pz);
        escaped = false; exitArmed=true; doorVisualDirty=true;
        motion.reset();
        bobOffset = 0;
        crouched = false;
        eyeHeight = 1.65f;
        fieldOfView = settings.fov;
        stamina.reset(); tension = 0;
        audio.reset(static_cast<uint32_t>(world.seed()));
        threat.reset(world.seed()); threat.setEnraged(world.keyCollected()); encounter.reset();
        mouse.reset(world.seed());mouseSqueaks=0;
        hiding=false; pendingKeyRoar=false; roarCooldown=0; occlusionClock=0; threatTransmission=1;
        threatAnimation=0; lastThreatMeshTime=-1;
        threatStoop=0; lastThreatStoop=-1; threatNoticeCooldown=0;
        threatGaitPhase=threatVisualSpeed=threatHeadYaw=threatHeadPitch=0; lastThreatContact={};
        threatBodyYaw=threat.yaw();
        travel = walk = shiftClock = 0;
        audio.update(0, 0, false, false, Surface::Carpet, true);
        notice = "RETURNED TO THE FIRST ROOM";
        noticeTime = 3;
        log("Returned to the first room.");
    };

    auto freshRun = [&](uint64_t seed) {
        world=World(seed);
        showHelp=false;guidePage=0;guidePending=(!options.hidden && !options.tour) || options.guideCheck;
        for(auto& [coord,mesh]:gpu) release(mesh);
        gpu.clear();
        returnToStart();
        exitUsed=false; doorsUsed=0; minimumStamina=100;
        notice.clear(); noticeTime=0; settingsOpen=false; menuSelection=0;
        input.pause(false); entered=true;
    };

    FrameQueue frameQueue;
    std::ofstream profile;
    if (!options.profile.empty()) {
        profile.open(options.profile);
        if (!profile) throw std::runtime_error("Cannot open frame profile: " + options.profile);
        profile << "frame,x,z,chunk_x,chunk_z,world_ms,upload_ms,frame_ms,uploads,lights,scene_ms,lens_ms,ui_ms,swap_ms,input_ms,move_ms,audio_ms,setup_ms,queue_wait_ms,resident_triangles,drawn_triangles,scare_elapsed,turn_weight,face_distance,fov,shake_x,shake_y,shake_z,shake_roll,look_x,look_y,look_z,threat_x,threat_z,body_yaw,head_yaw,forward_dot\n";
    }

    while (!glfwWindowShouldClose(window)) {
        const double frameStart = glfwGetTime();
        if (!options.unboundedQueue) frameQueue.wait();
        const double queueWaitMs = (glfwGetTime() - frameStart) * 1000;
        const double rawDelta = std::max(0.0, frameStart - lastTime);
        lastTime = frameStart;
        // Cap ordinary simulation stalls to avoid teleporting through a wall.
        // The automated walk has deterministic motion per frame, independent of
        // render speed; performance counters always use real wall-clock time.
        const float dt = (options.autoWalk || options.autoCrouch || options.autoDoor || options.autoExit || options.autoCatch || options.autoJumpscare || options.autoKey || options.autoHide || options.mouseFixture || options.guideCheck || options.forcedThreat) ? 1.f / 60.f :
                         static_cast<float>(std::min(rawDelta, .05));
        elapsed += dt;
        const double inputStart = glfwGetTime();
        glfwPollEvents();
        // Scripted checks use the same latched callbacks as physical keys.
        auto simulateKey = [&](int key) { Input::key(window,key,0,GLFW_PRESS,0); };
        std::optional<std::array<double,2>> scriptedMenuClick;
        if(options.autoSettings && frame<=2) simulateKey(frame==2 ? GLFW_KEY_LEFT : GLFW_KEY_DOWN);
        if(options.musicMenuCheck && frame<=7) simulateKey(frame==7 ? GLFW_KEY_ENTER : GLFW_KEY_DOWN);
        if(options.menuCheck && frame<=6) {
            const int keys[]={GLFW_KEY_DOWN,GLFW_KEY_ENTER,GLFW_KEY_DOWN,GLFW_KEY_RIGHT,GLFW_KEY_ESCAPE,GLFW_KEY_UP,GLFW_KEY_ENTER};
            if(!options.autoExit || frame<5) simulateKey(keys[frame]);
            else if(frame==5) simulateKey(GLFW_KEY_ESCAPE);
        }
        if(options.menuCheck && options.autoExit && frame==10) simulateKey(GLFW_KEY_ENTER);
        if(options.autoPause && frame==2) simulateKey(GLFW_KEY_ESCAPE);
        if(options.autoScareEscape && (frame==10 || frame==75)) simulateKey(GLFW_KEY_ESCAPE);
        if(options.autoJumpscare && (frame==2 || (options.autoRestart && frame==170))) simulateKey(GLFW_KEY_J);
        if((options.autoRestart || options.autoRestartClick) && encounter.gameOver() && frame>=90 && !scriptedRestarted) {
            if(options.autoRestartClick) {
                // Hidden windows cannot warp the OS pointer. Supply the same
                // framebuffer point consumed by the real menu hit test instead.
                int w,h; glfwGetFramebufferSize(window,&w,&h);
                scriptedMenuClick=std::array<double,2>{w*.5,(h-550)*.5+184};
                Input::button(window,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
            } else simulateKey(GLFW_KEY_ENTER);
            scriptedRestarted=true;
        }
        if(options.autoHide && (frame==5 || frame==95)) simulateKey(GLFW_KEY_E);
        if(options.guideCheck && frame==1 && !entered) simulateKey(GLFW_KEY_ENTER);
        if(options.guideCheck && (frame==5 || frame==7 || frame==9) && options.guideClose) simulateKey(GLFW_KEY_ENTER);
        if(options.guideCheck && (frame==5 || frame==7 || frame==9) && options.guideClick) Input::button(window,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        if(options.guideCheck && frame==8 && options.guideBack) simulateKey(GLFW_KEY_LEFT);
        if(options.guideCheck && frame==5 && options.guideSkip) simulateKey(GLFW_KEY_ESCAPE);
        input.poll();
        const double inputMs = (glfwGetTime() - inputStart) * 1000;
        const double moveStart = glfwGetTime();

        // Menus support both keyboard navigation and framebuffer-scaled clicks.
        auto saveSettings = [&]() {
            settings.sanitize(); input.sensitivity = settings.sensitivity;
            audio.setVolume(settings.volume);
            glfwSwapInterval(options.hidden ? 0 : int(settings.vsync));
            if ((!options.hidden || !options.settingsFile.empty()) && !settings.save(settingsPath)) {
                notice = "SETTINGS ACTIVE / COULD NOT SAVE FILE"; noticeTime = 4;
            }
        };
        bool resumeAction = false, newSeedAction = false, resetAction = false, quitAction = false;
        bool escapeHandled = false;
        const bool guideWasOpen=showHelp;
        if(showHelp) {
            int fw,fh,ww,wh;glfwGetFramebufferSize(window,&fw,&fh);glfwGetWindowSize(window,&ww,&wh);
            double mx,my;glfwGetCursorPos(window,&mx,&my);
            mx*=double(fw)/std::max(1,ww);my*=double(fh)/std::max(1,wh);
            const GuideLayout layout(fw,fh);
            if(options.guideCheck && options.guideClick && (frame==5 || frame==7 || frame==9)) {
                mx=layout.left+760*layout.scale;my=layout.top+506*layout.scale;
            }
            const bool next=input.hit(GLFW_KEY_ENTER) || input.hit(GLFW_KEY_RIGHT) ||
                (input.clicked && layout.contains(mx,my,652,484,216,44));
            const bool back=input.hit(GLFW_KEY_LEFT) ||
                (input.clicked && layout.contains(mx,my,32,486,126,40));
            const bool dismiss=input.hit(GLFW_KEY_ESCAPE) || input.hit(GLFW_KEY_H) ||
                (input.clicked && layout.contains(mx,my,838,14,38,32));
            if(dismiss || (next && guidePage==2)) {
                showHelp=false;input.pause(false);log("Controls popup closed.");
            } else if(next) { ++guidePage;log("Guide page: "+std::to_string(guidePage+1)); }
            else if(back && guidePage>0) { --guidePage;log("Guide page: "+std::to_string(guidePage+1)); }
            // Modal input must never also activate a menu or gameplay action.
            input.pressed.fill(false);input.clicked=false;escapeHandled=true;
        }
        if (input.paused && !encounter.scaring() && !guideWasOpen) {
            int menuW, menuH, windowW, windowH;
            glfwGetFramebufferSize(window, &menuW, &menuH);
            glfwGetWindowSize(window, &windowW, &windowH);
            double mouseX, mouseY; glfwGetCursorPos(window, &mouseX, &mouseY);
            mouseX *= double(menuW)/std::max(1,windowW); mouseY *= double(menuH)/std::max(1,windowH);
            if(scriptedMenuClick) { mouseX=(*scriptedMenuClick)[0]; mouseY=(*scriptedMenuClick)[1]; }
            const float boxW = std::min(700.f, float(menuW)-40), boxH = 550;
            const float left=(menuW-boxW)*.5f, top=(menuH-boxH)*.5f;
            if (settingsOpen) {
                if (input.hit(GLFW_KEY_UP)) settingSelection=(settingSelection+11)%12;
                if (input.hit(GLFW_KEY_DOWN)) settingSelection=(settingSelection+1)%12;
                int change = input.hit(GLFW_KEY_LEFT) ? -1 : (input.hit(GLFW_KEY_RIGHT) || input.hit(GLFW_KEY_ENTER)) ? 1 : 0;
                bool activate = input.hit(GLFW_KEY_ENTER);
                if (input.clicked && mouseX>=left+28 && mouseX<=left+boxW-28) {
                    int row=int(std::floor((mouseY-top-112)/29));
                    if (row>=0 && row<12) {
                        settingSelection=row;
                        activate = row>=10;
                        if (row==7) change=1;
                        if (mouseX>left+boxW-150) change=mouseX>left+boxW-85 ? 1 : -1;
                    }
                }
                if (change && settingSelection<10) {
                    switch(settingSelection) {
                    case 0: settings.sensitivity+=change*.01f; break;
                    case 1: settings.volume+=change*.05f; break;
                    case 2: settings.brightness+=change*.05f; break;
                    case 3: settings.fov+=change*2.f; break;
                    case 4: settings.headBob+=change*.1f; break;
                    case 5: settings.blur+=change*.1f; break;
                    case 6: settings.vsync=!settings.vsync; break;
                    case 7: settings.musicEnabled=!settings.musicEnabled; break;
                    case 8: settings.musicVolume+=change*.05f; break;
                    case 9: settings.ambientOcclusion+=change*.1f; break;
                    }
                    saveSettings(); fieldOfView=settings.fov;
                }
                if (activate && settingSelection==10) { settings=Settings{}; saveSettings(); fieldOfView=settings.fov; }
                if ((activate && settingSelection==11) || input.hit(GLFW_KEY_ESCAPE)) {
                    settingsOpen=false; escapeHandled=true; saveSettings();
                }
            } else {
                if (input.hit(GLFW_KEY_UP)) menuSelection=(menuSelection+4)%5;
                if (input.hit(GLFW_KEY_DOWN)) menuSelection=(menuSelection+1)%5;
                bool activate=input.hit(GLFW_KEY_ENTER);
                if(input.clicked && mouseX>=left+32 && mouseX<=left+boxW-32) {
                    int row=int(std::floor((mouseY-top-163)/52));
                    if(row>=0 && row<5) {menuSelection=row;activate=true;}
                }
                if(input.hit(GLFW_KEY_O)) {settingsOpen=true;activate=false;}
                if(activate) {
                    if(menuSelection==0) resumeAction=true;
                    if(menuSelection==1) settingsOpen=true;
                    if(menuSelection==2) newSeedAction=true;
                    if(menuSelection==3) resetAction=true;
                    if(menuSelection==4) quitAction=true;
                }
            }
        }
        // Input and focus management.
        if (input.hit(GLFW_KEY_ESCAPE) && !escapeHandled) {
            if(encounter.scaring()) encounter.skipScare();
            else if(!encounter.gameOver()) input.pause(!input.paused);
            if (input.paused) menuSelection=0;
            if (!input.paused) {
                entered = true;
                escaped = false;
            }
        }
        if ((resumeAction || (options.autoContinue && escaped)) && input.paused && !settingsOpen) {
            if(encounter.gameOver()) { freshRun(world.seed()); ++restarts; log("Restarted after capture with the same seed."); }
            else if(!encounter.scaring()) input.pause(false);
            entered = true;
            escaped = false;
        }
        if ((quitAction || input.hit(GLFW_KEY_Q)) && input.paused && !settingsOpen) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        if (!glfwGetWindowAttrib(window, GLFW_FOCUSED) &&
            !options.hidden && !options.tour && !input.paused) {
            input.pause(true);
            menuSelection=0;
        }
        if (input.hit(GLFW_KEY_F)) {
            flashlight = !flashlight;
        }
        if (input.hit(GLFW_KEY_C)) {
            clues = !clues;
        }
        if (input.hit(GLFW_KEY_TAB)) {
            showMap = !showMap;
        }
        if (input.hit(GLFW_KEY_H) && !input.paused && encounter.alive()) {
            showHelp=true;guidePage=0;input.pause(true);motion.reset();
        }
        if (input.hit(GLFW_KEY_V)) {
            cameraView = !cameraView;
        }
        if (input.hit(GLFW_KEY_F1)) {
            diagnostics = !diagnostics;
        }
        if (input.hit(GLFW_KEY_F2)) {
            wireframe = !wireframe;
        }
        if (input.hit(GLFW_KEY_F3)) {
            shifting = !shifting;
        }
        if (input.hit(GLFW_KEY_M)) {
            muted = !muted;
            if (muted) {
                log("Lamp shadow cache updates: "+std::to_string(lampShadowUpdates));
    audio.stop();
            } else {
                audio.start();
                log(audio.available() ? "Audio output active." : "Audio output unavailable; use M to retry.");
            }
        }
        if (((input.hit(GLFW_KEY_R) && !settingsOpen) || resetAction) && !encounter.scaring()) {
            if(encounter.gameOver()) {
                freshRun(world.seed());
                if(resetAction) { input.pause(true); entered=false; }
                else ++restarts;
            }
            else returnToStart();
        }
        if ((newSeedAction || input.hit(GLFW_KEY_N)) && input.paused && !settingsOpen && !encounter.scaring()) {
            freshRun(world.seed()+104729);
            notice="NEW SEED / NEW BUILDING"; noticeTime=3;
        }
        if(guidePending && entered && !input.paused && encounter.alive()) {
            guidePending=false;showHelp=true;guidePage=0;input.pause(true);motion.reset();
            log("Controls popup opened.");
        }
        input.smooth(dt);
        if (options.tour && encounter.alive() && !input.paused && !hiding) {
            input.setLook(options.yaw + std::sin(elapsed * .38) * 40, -3 + std::sin(elapsed * .2) * 4);
        }

        // Advance leaves before movement so collision uses their visible angle.
        const auto threatPosition=threat.active() ? std::optional<WorldPoint>{{threat.x(),threat.z()}} : std::nullopt;
        doorVisualDirty |= world.advanceDoors(dt,px,pz,.23,input.paused,threatPosition,Threat::RADIUS);
        // Move a circular player collider in small, axis-separated steps. This
        // permits sliding along walls while preventing tunnelling through them.
        const Vec3 forward = input.forward();
        bool moving = false;
        double frameDistance = 0;
        if (!input.paused) {
            crouched = hiding || world.headClearance(px,pz,.23)<1.8f || input.down(GLFW_KEY_LEFT_CONTROL) || input.down(GLFW_KEY_RIGHT_CONTROL) ||
                (options.autoCrouch && !(options.autoStand && frame >= options.frames / 2));
            const float targetHeight = crouched ? 1.08f : 1.65f;
            eyeHeight += (targetHeight - eyeHeight) * (1.f - std::exp(-dt * 12.f));
        }
        const bool sprinting = !input.paused && !crouched && stamina.canSprint() && (input.down(GLFW_KEY_LEFT_SHIFT) ||
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

        stamina.update(dt, sprinting && moving, input.paused);
        minimumStamina = std::min(minimumStamina, stamina.value);
        const double moveMs = (glfwGetTime() - moveStart) * 1000;
        const double worldStart = glfwGetTime();
        // Keep CPU and GPU working sets bounded as the player crosses chunks.
        world.update(px, pz);
        world.prepareAhead(px, pz); // One CPU mesh per frame, nearest first.
        maxChunks = std::max(maxChunks, world.chunks().size());
        maxPrepared = std::max(maxPrepared, world.preparedChunks().size());
        if (!input.paused && shifting) {
            shiftClock += dt;
            if (shiftClock > 14) {
                world.shiftBehind(px, pz, forward.x, forward.z,threatPosition);
                shiftClock = 0;
            }
        }
        const double worldMs = (glfwGetTime() - worldStart) * 1000;
        // Rearm only after returning to the front of the threshold, not when
        // closing the door or walking deeper into the exit vestibule.
        if(pz>world.exitLocation().z-.95) exitArmed=true;
        if(!input.paused && exitArmed && world.exitCrossed(px,pz)) {
            escaped=exitUsed=true; exitArmed=false;
            motion.reset(); input.pause(true); menuSelection=0;
            log("Exit reached by walking through the open doorway.");
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
        if(options.autoKey && frame==1) {
            const auto key=world.keyLocation(); px=key.x;pz=key.z+1;world.update(px,pz);
        }
        if(options.autoKey && options.autoExit && frame==20) {
            const auto exit=world.exitLocation();px=exit.x;pz=exit.z;world.update(px,pz);
        }
        if(!input.paused && !hiding && world.collectKey(px,pz)) {
            doorVisualDirty=true; threat.setEnraged(true); roarCooldown=7;
            pendingKeyRoar=true;
            notice="KEY FOUND / EXIT OPEN / IT HEARD YOU";noticeTime=5;
            log("Key collected. Exit unlocked. Creature enraged.");
        }
        HidingSpot nearbyHide{};
        bool nearHide=world.nearestHidingSpot(px,pz,1.3,nearbyHide);
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

        DoorInfo nearbyDoor{};
        bool nearDoor = world.nearestDoor(px, pz, 2.0, nearbyDoor);
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
        if (!input.paused && !hiding && !hideInteraction && (input.hit(GLFW_KEY_E) || (options.autoExit && frame == (options.menuCheck ? 8 : 2)) ||
            (options.autoDoor && frame==2) || (options.autoCloseDoor && frame==options.doorCloseFrame))) {
            if (nearDoor) {
                if (world.toggleDoor(nearbyDoor.id, px, pz)) {
                    doorVisualDirty=true;
                    ++doorsUsed; doorNoise=1;
                    nearbyDoor.open = !nearbyDoor.open;
                    audio.play(SoundKind::Door, {nearbyDoor.x, 1.2, nearbyDoor.z});
                    notice = nearbyDoor.open ? "OPENING DOOR" : "CLOSING DOOR";
                    log(notice + " / " + std::to_string(nearbyDoor.id));
                } else notice = nearbyDoor.isExit && !world.keyCollected()?"EXIT LOCKED / FIND THE BRASS KEY":"STEP CLEAR OF THE DOORWAY";
                noticeTime=2;
            } else {
                notice = "FACE A NEARBY DOOR TO INTERACT";
                noticeTime = 3;
            }
        }
        // Actual movement and explicit interaction noise feed one physical actor.
        // Menu time never advances its senses, path following, or grace period.
        const auto previousThreatState=threat.state();
        if(!input.paused) threatNoticeCooldown=std::max(0.f,threatNoticeCooldown-dt);
        if(options.autoCatch && frame==2 && encounter.alive()) {
            const Vec3 front=normalize(Vec3{forward.x,0,forward.z});
            double tx=px+front.x*1.35,tz=pz+front.z*1.35;
            if(world.blocked(tx,tz,Threat::RADIUS)) { tx=px; tz=pz; }
            threat.resetAt(tx,tz,std::atan2(-front.x,front.z));
        }
        ThreatPlayer perceivedPlayer{px,pz,forward.x,forward.z,dt>0 ? float(frameDistance/dt) : 0.f,crouched,doorNoise,hiding};
        if(options.mouseFixture && frame==2 && mouse.active) {
            px=mouse.x;pz=mouse.z;perceivedPlayer.x=px;perceivedPlayer.z=pz;
        }
        if(mouse.update(world,px,pz,dt,input.paused || !encounter.alive(),hiding)) {
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
        const double oldThreatX=threat.x(),oldThreatZ=threat.z();
        threat.setEnraged(world.keyCollected());
        if(!options.noThreat && !options.threatPreview)
            threat.update(world,perceivedPlayer,dt,input.paused || !encounter.alive());
        if(pendingKeyRoar && threat.active() && !input.paused) {
            audio.play(SoundKind::ThreatRoar,{threat.x(),1.8,threat.z()},1);
            pendingKeyRoar=false;roarCooldown=7;
        }
        if(threat.state()!=previousThreatState) {
            if(previousThreatState==ThreatState::Dormant) threatBodyYaw=threat.yaw();
            std::ostringstream threatEvent;
            threatEvent << "Threat: " << threatStateName(threat.state()) << " / distance "
                        << std::fixed << std::setprecision(2) << std::hypot(threat.x()-px,threat.z()-pz) << " m";
            log(threatEvent.str());
            if(threat.state()==ThreatState::Chase && threatNoticeCooldown<=0) {
                audio.play(SoundKind::ThreatNotice,{threat.x(),1.8,threat.z()},.8f);
                threatNoticeCooldown=5;
            }
        }
        if(!input.paused && encounter.alive() && world.keyCollected()) {
            roarCooldown-=dt;
            if(roarCooldown<=0 && threat.active()) {
                audio.play(SoundKind::ThreatRoar,{threat.x(),1.8,threat.z()},1);
                roarCooldown=7;
            }
        }
        const bool scareSuspended=!options.hidden && !glfwGetWindowAttrib(window,GLFW_FOCUSED);
        encounter.update(dt,scareSuspended);
        const bool debugScare=input.hit(GLFW_KEY_J) && encounter.alive() && !settingsOpen;
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
            ++catches; motion.reset(); moving=false; frameDistance=0;
            input.pause(true); settingsOpen=false; menuSelection=0;
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
                threatBodyYaw=std::hypot(travelX,travelZ)>.00001?
                    float(std::atan2(travelX,-travelZ)):threat.yaw();
                threatVisualSpeed+=(threat.speed()-threatVisualSpeed)*(1.f-std::exp(-dt*9.f));
                // Four planted feet share a distance-driven cycle. Integrating
                // distance avoids a phase jump when walk changes to run.
                const float stride=threatStrideLength(threatVisualSpeed);
                threatGaitPhase=std::fmod(threatGaitPhase+threat.speed()*dt/stride,1.f);
                const auto contacts=threatFootContacts(threatVisualSpeed,threatGaitPhase);
                if(threat.active() && threat.speed()>.15f) {
                    int landings=0;
                    for(std::size_t foot=0;foot<contacts.feet.size();++foot)
                        landings+=contacts.feet[foot]>.5f && lastThreatContact.feet[foot]<=.5f;
                    for(int landing=0;landing<landings;++landing) {
                        audio.play(SoundKind::ThreatStep,{threat.x(),.2,threat.z()},.8f);
                        ++threatFootfalls;
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

        const double audioStart = glfwGetTime();
        const auto currentRoom = roomInfo(int64_t(std::floor(px / CELL)), int64_t(std::floor(pz / CELL)), world.seed());
        const auto floorSurface = floorSurfaceAt(px, pz, world.seed());
        const Surface surface = floorSurface==FloorSurface::Tile ? Surface::HardFloor :
            floorSurface==FloorSurface::DampCarpet ? Surface::DampCarpet : Surface::Carpet;
        AudioScene soundScene;
        soundScene.listener={px,eyeHeight,pz};
        soundScene.forward={forward.x,forward.y,forward.z};
        double nearestSoundLight=1e30;
        float localIllumination=0, localInstability=0;
        for(const auto& [coord,chunk] : world.chunks()) for(const auto& lamp : chunk.lamps) {
            const double lx=coord.x*CHUNK_SIZE+lamp.x, lz=coord.z*CHUNK_SIZE+lamp.z;
            const double distance=(lx-px)*(lx-px)+(lz-pz)*(lz-pz)+(lamp.y-eyeHeight)*(lamp.y-eyeHeight);
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
        const float danger=std::max(environmentalDanger,threat.state()==ThreatState::Chase ? 1.f :
            threat.state()==ThreatState::Search ? .7f : 0.f);
        if(!input.paused) tension+=(danger-tension)*(1.f-std::exp(-dt*.6f));
        occlusionClock-=dt;
        if(occlusionClock<=0) {
            threatTransmission=threat.active()?world.soundTransmission(px,pz,threat.x(),threat.z()):1;
            occlusionClock=.1f;
        }
        soundScene.threatTransmission=threatTransmission;
        soundScene.tension=encounter.alive()?tension:0;
        if(!encounter.alive()) soundScene.lightPower=0;
        audio.setScene(soundScene);
        audio.update(dt, float(frameDistance), crouched, sprinting && moving, surface, input.paused && (!encounter.presenting() || scareSuspended || !scareConfig.audio));
        if (!input.paused) {
            // Widen only for real sprint travel, not for Shift held at a wall.
            // Exponential easing has the same response at different frame rates.
            const float targetFov = settings.fov + (sprinting && moving ? 4.f : 0.f);
            fieldOfView += (targetFov - fieldOfView) * (1.f - std::exp(-10.f * dt));
        }
        music.update(input.paused,muted || options.hidden || !settings.musicEnabled,settings.volume*settings.musicVolume,dt);
        if(!musicErrorReported && !music.error().empty()) {
            log("Background music unavailable: "+music.error()); musicErrorReported=true;
        }
        const double audioMs = (glfwGetTime() - audioStart) * 1000;
        const double uploadStart = glfwGetTime();
        unsigned uploads = 0;
        for (auto it = gpu.begin(); it != gpu.end();) {
            if (world.chunks().count(it->first) == 0 && world.preparedChunks().count(it->first) == 0) {
                release(it->second);
                it = gpu.erase(it);
            } else {
                ++it;
            }
        }
        size_t residentTriangles = 0;
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
            const auto posedCreature=threatModelVertices(options.threatPreview ? options.threatPreviewTime : threatAnimation,visualSpeed,
                visualSpeed>2.f,encounter.scaring() ? std::max(.001f,encounter.progress()) : 0.f,threatStoop,
                options.threatPreview ? -1.f : threatGaitPhase,
                options.threatPreview ? options.threatPreviewHeadYaw*PI/180 : threatHeadYaw,
                options.threatPreview ? options.threatPreviewHeadPitch*PI/180 : threatHeadPitch);
            uploadVertices(threatGpu,posedCreature,GL_DYNAMIC_DRAW);
            if(encounter.alive())threatHeadLocal=headCenter(posedCreature);
            lastThreatMeshTime=threatAnimation; lastScareMesh=encounter.scaring(); lastThreatStoop=threatStoop;
        }
        if(threat.active()) residentTriangles+=threatGpu.count/3;
        const double uploadMs = (glfwGetTime() - uploadStart) * 1000;
        const double setupStart = glfwGetTime();
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width <= 0 || height <= 0) {
            glfwWaitEventsTimeout(.02);
            continue;
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
        const auto origin = chunkAt(px, pz);
        const double originX = origin.x * CHUNK_SIZE;
        const double originZ = origin.z * CHUNK_SIZE;
        if(doorVisualDirty || !(origin==doorOrigin) || doorShift!=world.shifts()) {
            uploadVertices(doorGpu,world.doorVertices(originX,originZ),GL_DYNAMIC_DRAW);
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
        Vec3 cameraForward=forward;
        float shakeRoll=0;
        const auto scareShake=encounter.shake();
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
        const float renderFov=encounter.presenting() && scareConfig.fovPunch ?
            encounter.visualFov() : fieldOfView;
        const Mat4 projection = perspective(renderFov * PI / 180, float(width) / height, .06f, 110.f);
        const Mat4 vp = projection * view;

        struct NearLight {
            float distance;
            Vec3 position;
            float power;
            Vec3 color;
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
                shadow.set("uModel",translate(x,0,z));glBindVertexArray(mesh.vao);glDrawArrays(GL_TRIANGLES,0,mesh.count);
            }
            shadow.set("uModel",Mat4::identity());
            if(doorGpu.count){glBindVertexArray(doorGpu.vao);glDrawArrays(GL_TRIANGLES,0,doorGpu.count);}
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
        for(size_t i=0;i<lights.size();++i) {
            auto& cached=shadows.cache[i];const Vec3 pos=lights[i].position;
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
                    shadowRevision=(shadowRevision^std::uint64_t(door.openness*100000))*1099511628211ULL;
                }
            }
            if(!cached.valid||cached.revision!=shadowRevision||dot(pos-cached.position,pos-cached.position)>.000001f) {
                for(int face=0;face<6;++face){shadows.begin(int(i)*6+face);drawShadowGeometry(lampFace(pos,face),false);}
                cached={pos,shadowRevision,true};++lampShadowUpdates;
            }
        }
        glDisable(GL_POLYGON_OFFSET_FILL);glEnable(GL_CULL_FACE);glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER,target.fbo);glViewport(0,0,width,height);
        const double setupMs = (glfwGetTime() - setupStart) * 1000;
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
        // Match the shader's fixed light capacity and clear unused slots so
        // lighting from the previous frame cannot leak into this one.
        std::array<float, 72> lightPositions{}, lightColors{};
        std::array<float, 24> lightPowers{};
        for (size_t i = 0; i < lights.size(); ++i) {
            lightPositions[i * 3] = lights[i].position.x;
            lightPositions[i * 3 + 1] = lights[i].position.y;
            lightPositions[i * 3 + 2] = lights[i].position.z;
            lightColors[i * 3] = lights[i].color.x;
            lightColors[i * 3 + 1] = lights[i].color.y;
            lightColors[i * 3 + 2] = lights[i].color.z;
            lightPowers[i] = lights[i].power;
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
        int drawn = 0;
        size_t drawnTriangles = 0;
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
            glDrawArrays(GL_TRIANGLES, 0, mesh.count);
            ++drawn;
            drawnTriangles += mesh.count / 3;
        }
        uploadVertices(mouseGpu,mouse.vertices(originX,originZ),GL_DYNAMIC_DRAW);
        if(mouseGpu.count) {
            scene.set("uModel",translate(0,0,0));glBindVertexArray(mouseGpu.vao);
            glDrawArrays(GL_TRIANGLES,0,mouseGpu.count);drawnTriangles+=mouseGpu.count/3;
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
        const double sceneMs = (glfwGetTime() - sceneStart) * 1000;
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

        const double lensMs = (glfwGetTime() - lensStart) * 1000;
        const double uiStart = glfwGetTime();
        // Overlays use framebuffer pixels, so high-DPI rendering stays crisp.
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
        ui.text(36, 47, std::string("LEVEL 0 / ")+roomName(currentRoom.kind), 1.15f * scale, dim);
        const std::string seedText = "SEED " + std::to_string(world.seed());
        ui.text(float(width) - UI::textWidth(seedText, 1.25f * scale) - 26,
                28, seedText, 1.25f * scale, paper);

        if (!input.paused) {
            ui.rect(width * .5f - 1, height * .5f - 1, 2, 2, {.95f, .94f, .81f, .65f});
            ui.text(28, float(height) - 55,
                    "FLASHLIGHT " + std::string(flashlight && !hiding ? "ON" : "OFF") +
                    "   CLUES " + (clues ? "ON" : "OFF") +
                    (crouched ? "   CROUCHING" : ""), 1.2f * scale, paper);
            const auto door = world.exitLocation();
            const double dx = door.x - px, dz = door.z - pz;
            const double distance = std::hypot(dx, dz);
            const double ahead = dx * forward.x + dz * forward.z;
            const double side = -dx * forward.z + dz * forward.x;
            const std::string bearing = ahead > distance * .75 ? "AHEAD" :
                ahead < -distance * .75 ? "BEHIND" : side > 0 ? "RIGHT" : "LEFT";
            const std::string signal = distance < 10000 ? std::to_string(int(distance)) + " M / " + bearing : "DISTANT / " + bearing;
            ui.text(28, float(height) - 80, "EXIT SIGNAL " + signal, 1.25f * scale, {.47f, .88f, .68f, 1});
            const auto key=world.keyLocation();
            const double kx=key.x-px,kz=key.z-pz,kd=std::hypot(kx,kz);
            const double ka=kx*forward.x+kz*forward.z,ks=-kx*forward.z+kz*forward.x;
            const std::string kb=ka>kd*.75?"AHEAD":ka<-kd*.75?"BEHIND":ks>0?"RIGHT":"LEFT";
            ui.text(28,float(height)-104,world.keyCollected()?"KEY COLLECTED / EXIT OPEN / CREATURE ENRAGED":
                "KEY SIGNAL "+std::to_string(int(kd))+" M / "+kb,1.25f*scale,gold);
            if(hiding || nearHide) ui.text(width*.5f-170,height*.5f+58,
                hiding?"HIDDEN / E TO LEAVE":"E / HIDE IN WALL RECESS",1.3f*scale,paper);
            ui.text(float(width)-212, float(height)-70, stamina.exhausted ? "CATCH YOUR BREATH" : "STAMINA", 1.15f*scale, stamina.exhausted ? gold : dim);
            ui.rect(float(width)-212, float(height)-49, 180, 5, {.08f,.09f,.06f,.9f});
            ui.rect(float(width)-212, float(height)-49, 180*stamina.value/100.f, 5, gold);
            if (nearDoor) {
                const std::string prompt = nearbyDoor.isExit && !world.keyCollected()?"EXIT LOCKED / FIND KEY" : nearbyDoor.isExit && nearbyDoor.openness>.92f ? "WALK THROUGH TO EXIT / E CLOSE" : nearbyDoor.open ? "E / CLOSE DOOR" : nearbyDoor.isExit ? "E / OPEN EXIT DOOR" : "E / OPEN DOOR";
                const float promptW = UI::textWidth(prompt, 1.8f * scale);
                ui.rect(width * .5f - promptW * .5f - 16, height * .63f - 12, promptW + 32, 38, {.025f, .1f, .065f, .94f});
                ui.text(width * .5f - promptW * .5f, height * .63f, prompt, 1.8f * scale, {.65f, 1, .8f, 1});
            }
        }
        }
        if (showMap && !input.paused) {
            minimap(ui, world, px, pz, forward, width);
        }
        if (diagnostics && !encounter.scaring()) {
            std::ostringstream text;
            text << "FPS " << int(fps)
                 << "\nRESIDENT TRIANGLES " << residentTriangles << " / DRAWN " << drawnTriangles
                 << "\nCHUNKS " << world.chunks().size() << " / DRAWN " << drawn
                 << "\nCELL " << int64_t(std::floor(px / CELL)) << ", " << int64_t(std::floor(pz / CELL))
                 << "\nDISTANCE " << int(travel) << " M / SHIFTS " << world.shifts()
                 << "\nLIGHTS " << lights.size() << " / SPEED " << std::fixed << std::setprecision(2) << motion.speed()
                 << "\nROOM " << currentRoom.widthCells * CELL << " X " << currentRoom.depthCells * CELL
                 << " M / CEILING " << currentRoom.height << " M"
                 << "\nEYE " << std::fixed << std::setprecision(2) << eyeHeight << " M / " << (crouched ? "CROUCH" : "STAND")
                 << "\nTHREAT " << threatStateName(threat.state()) << " / PATH NODES " << threat.pathNodesVisited();
            ui.rect(26, 83, 480, 182, {.02f, .025f, .02f, .82f});
            ui.text(38, 95, text.str(), 1.4f, paper);
        }
        if (noticeTime > 0) {
            noticeTime -= dt;
            const float textWidth = UI::textWidth(notice, 1.5f);
            ui.rect(width * .5f - textWidth * .5f - 12, 84, textWidth + 24, 30,
                    {.02f, .025f, .02f, .85f});
            ui.text(width * .5f - textWidth * .5f, 94, notice, 1.5f, gold);
        }
        if (input.paused && !encounter.scaring() && !showHelp) {
            ui.rect(0,0,float(width),float(height),{.014f,.018f,.011f,.70f});
            const float boxW=std::min(700.f,float(width)-40), boxH=550;
            const float left=(width-boxW)*.5f, top=(height-boxH)*.5f;
            ui.rect(left,top,boxW,boxH,{.032f,.039f,.027f,.96f});
            ui.rect(left,top,boxW,2,gold);
            ui.text(left+32,top+27,settingsOpen ? "CAMERA / SOUND / DISPLAY" : "LEVEL 0 / ENDLESS EXPLORATION",1.4f,gold);
            ui.text(left+32,top+57,settingsOpen ? "SETTINGS" : encounter.gameOver() ? "GAME OVER" : escaped ? "YOU ESCAPED" : entered ? "PAUSED" : "BACKROOMS",3.6f,paper);
            if (settingsOpen) {
                const std::array<std::string,12> labels={"MOUSE SENSITIVITY","MASTER VOLUME","BRIGHTNESS","FIELD OF VIEW","HEAD BOB","CAMERA SOFTNESS","VSYNC","MUSIC","MUSIC VOLUME","CONTACT SHADING","RESET DEFAULTS","BACK"};
                auto decimal=[](float value,int precision) {std::ostringstream out;out<<std::fixed<<std::setprecision(precision)<<value;return out.str();};
                const std::array<std::string,10> values={decimal(settings.sensitivity,3),std::to_string(int(settings.volume*100+.5f))+"%",std::to_string(int(settings.brightness*100+.5f))+"%",std::to_string(int(settings.fov)),std::to_string(int(settings.headBob*100+.5f))+"%",std::to_string(int(settings.blur*100+.5f))+"%",settings.vsync?"ON":"OFF",settings.musicEnabled?"ON":"OFF",std::to_string(int(settings.musicVolume*100+.5f))+"%",std::to_string(int(settings.ambientOcclusion*100+.5f))+"%"};
                for(int i=0;i<12;++i) {
                    float y=top+112+i*29;
                    const bool selected=i==settingSelection;
                    ui.rect(left+28,y,boxW-56,27,selected ? Color{.19f,.21f,.13f,1} : Color{.048f,.055f,.037f,1});
                    ui.text(left+42,y+10,labels[i],1.5f,selected?gold:paper);
                    if(i<10) {
                        ui.text(left+boxW-260,y+10,values[i],1.4f,paper);
                        ui.text(left+boxW-126,y+9,"-",1.8f,gold);
                        ui.text(left+boxW-65,y+9,"+",1.8f,gold);
                    }
                }
                ui.text(left+32,top+boxH-68,music.available() ? "MUSIC: LOADED" : "MUSIC: NO TRACK LOADED",1.1f,dim);
                ui.text(left+32,top+boxH-47,"UP/DOWN SELECT   LEFT/RIGHT ADJUST   ESC BACK",1.3f,dim);
                ui.text(left+32,top+boxH-25,"CLICK A ROW OR ITS - / + CONTROLS. SETTINGS SAVE AUTOMATICALLY.",1.1f,dim);
            } else {
                ui.text(left+32,top+107,encounter.gameOver() ? "IT FOUND YOU. BREAK SIGHT. CROUCH TO STAY QUIET." : escaped ? "THE EXIT IS OPEN. YOU CAN KEEP EXPLORING." : "FOLLOW THE HUM. LISTEN FOR FOOTSTEPS THAT ARE NOT YOURS.",1.35f,dim);
                const std::array<std::string,5> labels={encounter.gameOver()?"RESTART RUN":escaped?"CONTINUE EXPLORING":entered?"RESUME":"ENTER THE BACKROOMS","SETTINGS","NEW SEEDED BUILDING",encounter.gameOver()?"MAIN MENU":"RETURN TO FIRST ROOM","QUIT"};
                for(int i=0;i<5;++i) {
                    float y=top+163+i*52; const bool selected=i==menuSelection;
                    ui.rect(left+32,y,boxW-64,43,selected ? Color{.69f,.59f,.31f,1} : Color{.08f,.09f,.058f,1});
                    ui.text(left+50,y+14,labels[i],1.7f,selected?Color{.055f,.065f,.035f,1}:paper);
                }
                ui.text(left+32,top+472,encounter.gameOver()?"CLICK RESTART OR PRESS ENTER   O SETTINGS   Q QUIT":"CLICK OR UP/DOWN + ENTER   O SETTINGS   ESC RESUME",1.2f,dim);
                ui.text(left+32,top+boxH-27,"CGF / OPENGL 3.3 / THRESHOLD VERSION 0.12",1.1f,dim);
            }
        }
        if(showHelp) drawControlsGuide(ui,width,height,guidePage);
        ui.render(width, height);

        // Capture before swapping, while the completed image is in GL_BACK.
        if (input.hit(GLFW_KEY_F12)) {
            const auto stamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            screenshot(fs::path("captures") / ("backrooms-" + std::to_string(stamp) + ".bmp"),
                       width, height);
            notice = "SCREENSHOT SAVED IN CAPTURES";
            noticeTime = 3;
        }
        const bool lastFrame = options.frames > 0 && frame + 1 >= options.frames;
        if (lastFrame && !options.capture.empty()) {
            screenshot(options.capture, width, height);
        }
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            throw std::runtime_error("OpenGL error " + std::to_string(error) +
                                     " at frame " + std::to_string(frame));
        }
        const double uiMs = (glfwGetTime() - uiStart) * 1000;
        if (!options.unboundedQueue) frameQueue.submit();
        const double swapStart = glfwGetTime();
        glfwSwapBuffers(window);
        const double swapMs = (glfwGetTime() - swapStart) * 1000;
        ++frame;

        // Measure complete rendered frames, including swap/vsync. These real
        // durations are never clamped to the physics time-step limit.
        const double frameSeconds = glfwGetTime() - frameStart;
        maxFrameMs = std::max(maxFrameMs, frameSeconds * 1000);
        if (profile) profile << frame << "," << px << "," << pz << "," << origin.x << "," << origin.z << ","
                             << worldMs << "," << uploadMs << "," << frameSeconds * 1000 << "," << uploads << "," << lights.size() << "," << sceneMs << "," << lensMs << "," << uiMs << "," << swapMs << "," << inputMs << "," << moveMs << "," << audioMs << "," << setupMs << "," << queueWaitMs << "," << residentTriangles << "," << drawnTriangles
            << "," << encounter.elapsed() << "," << encounter.cameraTurn()
            << "," << (1.65f+(scareConfig.faceDistance-1.65f)*(scareConfig.lunge?encounter.lunge():0))
            << "," << renderFov << "," << scareShake.x << "," << scareShake.y << "," << scareShake.z << "," << shakeRoll
            << "," << cameraForward.x << "," << cameraForward.y << "," << cameraForward.z
            << "," << threat.x() << "," << threat.z() << "," << threatBodyYaw << "," << threatHeadYaw
            << "," << (std::hypot(threat.x()-oldThreatX,threat.z()-oldThreatZ)>.00001?
                ((threat.x()-oldThreatX)*std::sin(threatBodyYaw)-(threat.z()-oldThreatZ)*std::cos(threatBodyYaw))/std::hypot(threat.x()-oldThreatX,threat.z()-oldThreatZ):1.) << "\n";
        fpsTime += frameSeconds;
        ++fpsFrames;
        if (fpsTime >= .5) {
            fps = static_cast<float>(fpsFrames / fpsTime);
            fpsTime = 0;
            fpsFrames = 0;
        } else if (frame == 1 && frameSeconds > 0) {
            fps = static_cast<float>(1.0 / frameSeconds);
        }
        if (lastFrame) {
            break;
        }
    }

    const double runtime = glfwGetTime() - runStart;
    const double averageFps = runtime > 0 ? frame / runtime : 0;
    std::ostringstream summary;
    summary << std::fixed << std::setprecision(2)
            << "Completed " << frame << " frames. Max resident chunks: " << maxChunks
            << ". Average FPS: " << averageFps << ". Last FPS: " << fps
            << ". Max frame ms: " << maxFrameMs << ". Position: " << px << ", " << pz
            << ". Shifts: " << world.shifts() << ". Room: " << roomName(roomInfo(int64_t(std::floor(px/CELL)),int64_t(std::floor(pz/CELL)),world.seed()).kind)
            << ". Exit used: " << (exitUsed ? "yes" : "no")
            << ". Escaped: " << (escaped ? "yes" : "no")
            << ". Eye height: " << eyeHeight << ". Crouched: " << (crouched ? "yes" : "no")
            << ". FOV: " << fieldOfView << ". Velocity: " << motion.speed()
            << ". Stamina: " << stamina.value << ". Minimum stamina: " << minimumStamina
            << ". Doors used: " << doorsUsed << ". Tension: " << tension
            << ". Creature footfalls: " << threatFootfalls << ". Head yaw: " << threatHeadYaw*180/PI
            << ". Threat: " << threatStateName(threat.state()) << ". Catches: " << catches << ". Restarts: " << restarts
            << ". Mouse squeaks: " << mouseSqueaks << ". Guide: " << (showHelp?"open":"closed")
            << ". Guide page: " << guidePage+1
            << ". Key: " << (world.keyCollected()?"collected":"missing") << ". Hiding: " << (hiding?"yes":"no")
            << ". Encounter: " << (encounter.scaring()?"jumpscare":encounter.gameOver()?"game-over":"exploring")
            << ". Music loaded: " << (musicLoaded ? "yes" : "no")
            << ". Music enabled: " << (settings.musicEnabled ? "yes" : "no")
            << ". Brightness: " << settings.brightness << ". Volume: " << settings.volume
            << ". Paused: " << (input.paused ? "yes" : "no") << ". GL errors: 0.";
    log(summary.str());
    log("Streaming: max prepared chunks " + std::to_string(maxPrepared) +
        ", cached promotions " + std::to_string(world.streamPromotions()) +
        ", synchronous fallback chunks " + std::to_string(world.streamFallbacks()) + ".");

    // Delete GL objects while their context is still current.
    frameQueue.destroy();
    log("Lamp shadow cache updates: "+std::to_string(lampShadowUpdates));
    audio.stop();
    music.stop(); release(mouseGpu); release(keyGpu); release(doorGpu); release(threatGpu);
    for (auto& [coord, mesh] : gpu) {
        release(mesh);
    }
    destroyTextures(textures);
    ui.destroy();
    scene.destroy();
    lens.destroy();
    target.destroy();ambientTarget.destroy();shadows.destroy();shadow.destroy();ao.destroy();
    glDeleteVertexArrays(1, &emptyVao);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

int main(int argc, char** argv) {
    try {
#ifdef _WIN32
        // Windows argv uses a narrow encoding that cannot represent every path.
        std::wstring executableName(32768, L'\0');
        const DWORD executableLength = GetModuleFileNameW(nullptr, executableName.data(),
                                                         static_cast<DWORD>(executableName.size()));
        if (executableLength == 0 || executableLength >= executableName.size()) {
            throw std::runtime_error("Could not determine the executable path.");
        }
        executableName.resize(executableLength);
        const fs::path executable(executableName);
#else
        const fs::path executable = fs::absolute(fs::path(argv[0]));
#endif
        // Shader paths are relative to the executable/project root.
        if (fs::exists(executable.parent_path() / "vshader_backrooms.glsl")) {
            fs::current_path(executable.parent_path());
        }
        logFile.open("backrooms.log", std::ios::trunc);
        const Options options = parseOptions(argc, argv);
        log(loadMonsterFootsteps()?"Loaded supplied heavy footsteps.":"Heavy footsteps missing; using fallback.");
        log(loadJumpscareSample()?"Loaded supplied jumpscare roar.":"Jumpscare roar unavailable; using synthesized fallback.");
        log("Backrooms / Threshold v0.12 | seed " + std::to_string(options.seed));
        if (loadFootstepSamples()) {
            log("Loaded " + std::to_string(recordedFootstepCount()) + " recorded footstep samples.");
        } else {
            log("Recorded footsteps missing or invalid; using procedural fallback. Keep assets/audio/footsteps beside the executable.");
        }
        if (!options.audioPreview.empty()) {
            const fs::path path(options.audioPreview);
            if (path.has_parent_path()) fs::create_directories(path.parent_path());
            if (!writeAudioPreview(path.string())) throw std::runtime_error("Could not export audio preview.");
            log("Recorded and synthesized stereo sound preview saved: " + path.string());
            return 0;
        }
        return run(options);
    } catch (const std::exception& error) {
        log(std::string("ERROR: ") + error.what());
#ifdef _WIN32
        bool hidden = false;
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--hidden") {
                hidden = true;
            }
        }
        if (!hidden) {
            MessageBoxA(nullptr, error.what(), "Backrooms could not start", MB_OK | MB_ICONERROR);
        }
#endif
        glfwTerminate();
        return 1;
    }
}
