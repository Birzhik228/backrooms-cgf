#pragma once
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

namespace br {
struct Settings {
    float sensitivity = .095f, volume = .75f, brightness = 1.f;
    float fov = 78.f, headBob = 1.f, blur = 1.f;
    float musicVolume = .35f;
    bool vsync = true, musicEnabled = true;
    static float valid(float n, float fallback, float lo, float hi) {
        return std::isfinite(n) ? std::clamp(n, lo, hi) : fallback;
    }
    void sanitize() {
        sensitivity = valid(sensitivity, .095f, .025f, .25f);
        volume = valid(volume, .75f, 0.f, 1.f);
        brightness = valid(brightness, 1.f, .6f, 1.5f);
        fov = valid(fov, 78.f, 65.f, 100.f);
        headBob = valid(headBob, 1.f, 0.f, 1.f);
        blur = valid(blur, 1.f, 0.f, 1.5f);
        musicVolume = valid(musicVolume, .35f, 0.f, 1.f);
    }
    bool load(const std::string& path) {
        std::ifstream file(path);
        if (!file) return false;
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream row(line);
            std::string key; float value;
            if (!(row >> key >> value) || !std::isfinite(value)) continue;
            if (key == "sensitivity") sensitivity = value;
            else if (key == "volume") volume = value;
            else if (key == "brightness") brightness = value;
            else if (key == "fov") fov = value;
            else if (key == "headBob") headBob = value;
            else if (key == "blur") blur = value;
            else if (key == "musicVolume") musicVolume = value;
            else if (key == "musicEnabled") musicEnabled = value != 0;
            else if (key == "vsync") vsync = value != 0;
        }
        sanitize();
        return true;
    }
    bool save(const std::string& path) const {
        std::ofstream file(path);
        file << "sensitivity " << sensitivity << "\nvolume " << volume
             << "\nbrightness " << brightness << "\nfov " << fov
             << "\nheadBob " << headBob << "\nblur " << blur
             << "\nmusicVolume " << musicVolume << "\nmusicEnabled " << int(musicEnabled)
             << "\nvsync " << int(vsync) << '\n';
        file.flush();
        return bool(file);
    }
};
}
