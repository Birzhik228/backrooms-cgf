// Application entry point; gameplay and presentation live in app/.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "app/diagnostics.h"
#include "app/options.h"
#include "app/game.h"
#include "common/audio.h"
#include "common/balance.h"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace br;
using namespace br::app;

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
        openLog("backrooms.log");
        const Options options = parseOptions(argc, argv);
        const auto tuning=loadBalance(options.balanceFile);
        log(tuning.loaded?"Balance configuration loaded.":"Using built-in balance defaults.");
        for(const auto& diagnostic:tuning.diagnostics)log("Balance: "+diagnostic);
        log(loadMonsterFootsteps()?"Loaded supplied heavy footsteps.":"Heavy footsteps missing; using fallback.");
        log(loadJumpscareSample()?"Loaded supplied jumpscare roar.":"Jumpscare roar unavailable; using synthesized fallback.");
        log(loadAraneaSamples()?"Loaded Aranea audio: 16 walk, run, visual notice and roar samples.":
            "Aranea audio bank missing or invalid; using legacy creature sounds.");
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
        return br::app::run(options);
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
        return 1; // WindowContext already unwound before this handler.
    }
}
