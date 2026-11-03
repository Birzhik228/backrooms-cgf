#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <istream>
#include <locale>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace br {

// Gameplay values use the original numeric types/defaults. Geometry, collider
// sizes, path-search budgets and rendering constants deliberately stay fixed.
struct Balance {
    float playerWalkSpeed = 2.5f;
    float playerSprintSpeed = 4.0f;
    float playerCrouchSpeed = 1.1f;
    float playerAcceleration = 8.f;
    float playerBraking = 18.f;
    float staminaDrain = 12.f;
    float staminaRecovery = 14.f;
    float staminaDelay = 1.5f;
    float staminaResumeThreshold = 22.f;
    float araneaWanderSpeed = 1.2f;
    float araneaChaseSpeed = 3.6f;
    float araneaEnragedMultiplier = 1.35f;
    float araneaSpawnGrace = 8.f;
    double araneaSpawnMin = 24.0;
    double araneaSpawnMax = 32.0;
    float araneaSightRange = 22.f;
    float araneaMemory = 5.f;
    double araneaCatchDistance = .62;
    double araneaMotionThreshold = .12;
    double araneaHearingBase = 2.0;
    double araneaHearingPerSpeed = 3.5;
    double araneaHearingMax = 18.0;
    double araneaCrouchHearing = .28;
    double araneaDoorHearing = 20.0;
    double araneaWallHearing = .45;
    double araneaAlarmRange = 48.0;
    float araneaAlarmMemory = 5.f;
    float araneaWalkTurnRate = 6.f;
    float araneaChaseTurnRate = 9.f;
    double mouseApproachSpeed = 1.65;
    double mouseFleeSpeed = 2.1;
    float partygoerSpeed = 4.875f;
    float partygoerAlarmSpeed = 7.5f;
    float partygoerAlarmDuration = 6.f;
    float darknessChaseSpeed = 4.8f;
    float darknessFlashlightRange = 18.f;
    double darknessCatchDistance = .65;
};

struct BalanceParseResult {
    Balance values{};
    std::vector<std::string> diagnostics;
    bool valid() const { return diagnostics.empty(); }
};

struct BalanceLoadResult {
    bool loaded = false;
    bool usedDefaults = true;
    bool alreadyInitialized = false;
    std::vector<std::string> diagnostics;
};

namespace balance_detail {
struct Field {
    const char* key;
    double minimum, maximum;
    void (*assign)(Balance&, double);
};

template<auto Member>
inline Field field(const char* key, double minimum, double maximum) {
    return {key, minimum, maximum, [](Balance& values, double value) {
        values.*Member = static_cast<std::decay_t<decltype(values.*Member)>>(value);
    }};
}

inline const auto& fields() {
    static const std::array<Field, 37> schema{{
        field<&Balance::playerWalkSpeed>("player.walk_speed", 0.5, 6),
        field<&Balance::playerSprintSpeed>("player.sprint_speed", 0.5, 10),
        field<&Balance::playerCrouchSpeed>("player.crouch_speed", 0.1, 3),
        field<&Balance::playerAcceleration>("player.acceleration", 0.5, 40),
        field<&Balance::playerBraking>("player.braking", 0.5, 60),
        field<&Balance::staminaDrain>("stamina.drain_per_second", 0.1, 100),
        field<&Balance::staminaRecovery>("stamina.recovery_per_second", 0.1, 100),
        field<&Balance::staminaDelay>("stamina.recovery_delay", 0, 10),
        field<&Balance::staminaResumeThreshold>("stamina.resume_threshold", 1, 100),
        field<&Balance::araneaWanderSpeed>("aranea.wander_speed", 0.1, 5),
        field<&Balance::araneaChaseSpeed>("aranea.chase_speed", 0.1, 10),
        field<&Balance::araneaEnragedMultiplier>("aranea.enraged_multiplier", 1, 2),
        field<&Balance::araneaSpawnGrace>("aranea.spawn_grace", 0, 120),
        field<&Balance::araneaSpawnMin>("aranea.spawn_min_distance", 8, 60),
        field<&Balance::araneaSpawnMax>("aranea.spawn_max_distance", 8, 60),
        field<&Balance::araneaSightRange>("aranea.sight_range", 1, 24),
        field<&Balance::araneaMemory>("aranea.memory_seconds", 0.1, 60),
        field<&Balance::araneaCatchDistance>("aranea.catch_distance", 0.5, 1),
        field<&Balance::araneaMotionThreshold>("aranea.hearing_motion_threshold", 0.01, 2),
        field<&Balance::araneaHearingBase>("aranea.hearing_base", 0, 12),
        field<&Balance::araneaHearingPerSpeed>("aranea.hearing_per_speed", 0, 10),
        field<&Balance::araneaHearingMax>("aranea.hearing_max", 0, 48),
        field<&Balance::araneaCrouchHearing>("aranea.crouch_hearing_multiplier", 0, 1),
        field<&Balance::araneaDoorHearing>("aranea.door_hearing_range", 0, 48),
        field<&Balance::araneaWallHearing>("aranea.wall_hearing_multiplier", 0, 1),
        field<&Balance::araneaAlarmRange>("aranea.alarm_range", 1, 100),
        field<&Balance::araneaAlarmMemory>("aranea.alarm_memory_seconds", 0.1, 60),
        field<&Balance::araneaWalkTurnRate>("aranea.walk_turn_rate", 0.5, 12),
        field<&Balance::araneaChaseTurnRate>("aranea.chase_turn_rate", 0.5, 18),
        field<&Balance::mouseApproachSpeed>("mouse.approach_speed", 0.1, 4.5),
        field<&Balance::mouseFleeSpeed>("mouse.flee_speed", 0.1, 4.5),
        field<&Balance::partygoerSpeed>("partygoer.speed", 0.1, 10),
        field<&Balance::partygoerAlarmSpeed>("partygoer.alarm_speed", 0.1, 12),
        field<&Balance::partygoerAlarmDuration>("partygoer.alarm_seconds", 0.1, 30),
        field<&Balance::darknessChaseSpeed>("darkness.chase_speed", 0.1, 6),
        field<&Balance::darknessFlashlightRange>("darkness.flashlight_range", 1, 30),
        field<&Balance::darknessCatchDistance>("darkness.catch_distance", 0.5, 1),
    }};
    return schema;
}

inline std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

struct RunConfiguration {
    Balance values{};
    bool initialized = false;
    bool usingDefaults = true;
};
inline RunConfiguration& storage() {
    static RunConfiguration config;
    return config;
}
} // namespace balance_detail

// A pure parser: callers can inspect a proposed configuration without changing
// the running game. Any error rejects the entire candidate, never a partial set.
inline BalanceParseResult parseBalance(std::istream& input,
                                       const std::string& source = "balance.cfg") {
    BalanceParseResult result;
    Balance candidate;
    std::map<std::string, std::size_t> seen;
    std::size_t lineNumber = 0;
    auto error = [&](std::size_t line, const std::string& message) {
        result.diagnostics.push_back(source + ":" + std::to_string(line) + ": " + message);
    };
    std::string line;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (lineNumber > 256 || line.size() > 4096) {
            error(lineNumber, "configuration exceeds 256 lines or 4096 characters per line");
            break;
        }
        if (lineNumber == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        const auto comment = line.find('#');
        if (comment != std::string::npos) line.erase(comment);
        line = balance_detail::trim(line);
        if (line.empty()) continue;
        const auto equals = line.find('=');
        if (equals == std::string::npos || line.find('=', equals + 1) != std::string::npos) {
            error(lineNumber, "expected one key=value assignment");
            continue;
        }
        const auto key = balance_detail::trim(line.substr(0, equals));
        const auto text = balance_detail::trim(line.substr(equals + 1));
        if (key.empty() || text.empty()) {
            error(lineNumber, "key and value must not be empty");
            continue;
        }
        const auto previous = seen.find(key);
        if (previous != seen.end()) {
            error(lineNumber, "duplicate key '" + key + "' (first set at line " +
                  std::to_string(previous->second) + ")");
            continue;
        }
        seen.emplace(key, lineNumber);
        const balance_detail::Field* selected = nullptr;
        for (const auto& field : balance_detail::fields()) {
            if (key == field.key) { selected = &field; break; }
        }
        if (!selected) {
            error(lineNumber, "unknown key '" + key + "'");
            continue;
        }
        std::istringstream number(text);
        number.imbue(std::locale::classic());
        double value = 0;
        number >> value;
        if (!number || !std::isfinite(value)) {
            error(lineNumber, "'" + key + "' requires a finite decimal number");
            continue;
        }
        number >> std::ws;
        if (!number.eof()) {
            error(lineNumber, "unexpected text after the value for '" + key + "'");
            continue;
        }
        if (value < selected->minimum || value > selected->maximum) {
            std::ostringstream range;
            range.imbue(std::locale::classic());
            range << "'" << key << "' must be between " << selected->minimum
                  << " and " << selected->maximum;
            error(lineNumber, range.str());
            continue;
        }
        selected->assign(candidate, value);
    }
    if (input.bad()) error(lineNumber, "failed while reading configuration");
    auto relation = [&](bool valid, const char* key, const std::string& message) {
        if (!valid) {
            const auto found = seen.find(key);
            error(found == seen.end() ? 0 : found->second, message);
        }
    };
    relation(candidate.playerCrouchSpeed <= candidate.playerWalkSpeed,
             "player.crouch_speed", "player.crouch_speed must not exceed player.walk_speed");
    relation(candidate.playerWalkSpeed <= candidate.playerSprintSpeed,
             "player.sprint_speed", "player.sprint_speed must be at least player.walk_speed");
    relation(candidate.playerBraking >= candidate.playerAcceleration,
             "player.braking", "player.braking must be at least player.acceleration");
    relation(candidate.araneaWanderSpeed <= candidate.araneaChaseSpeed,
             "aranea.chase_speed", "aranea.chase_speed must be at least aranea.wander_speed");
    relation(candidate.araneaChaseSpeed * candidate.araneaEnragedMultiplier <= 12.f,
             "aranea.enraged_multiplier", "aranea.chase_speed * aranea.enraged_multiplier must not exceed 12 m/s");
    relation(candidate.araneaSpawnMin <= candidate.araneaSpawnMax,
             "aranea.spawn_max_distance", "aranea.spawn_max_distance must be at least aranea.spawn_min_distance");
    relation(candidate.araneaHearingMax >= candidate.araneaHearingBase,
             "aranea.hearing_max", "aranea.hearing_max must be at least aranea.hearing_base");
    relation(candidate.araneaChaseTurnRate >= candidate.araneaWalkTurnRate,
             "aranea.chase_turn_rate", "aranea.chase_turn_rate must be at least aranea.walk_turn_rate");
    relation(candidate.mouseFleeSpeed >= candidate.mouseApproachSpeed,
             "mouse.flee_speed", "mouse.flee_speed must be at least mouse.approach_speed");
    relation(candidate.partygoerAlarmSpeed >= candidate.partygoerSpeed,
             "partygoer.alarm_speed", "partygoer.alarm_speed must be at least partygoer.speed");
    if (result.valid()) result.values = candidate;
    return result;
}

// Read-only during simulation. The default object also lets standalone native
// tests exercise the unchanged game without loading files or creating a window.
inline const Balance& balance() { return balance_detail::storage().values; }

// Call once before constructing gameplay objects. Editing a file affects the
// next process launch, never an in-progress chase or audio/physics thread.
inline BalanceLoadResult loadBalance(const std::filesystem::path& path) {
    auto& state = balance_detail::storage();
    BalanceLoadResult result;
    if (state.initialized) {
        result.alreadyInitialized = true;
        result.usedDefaults = state.usingDefaults;
        result.diagnostics.push_back("Balance configuration is already fixed for this run; restart to reload it.");
        return result;
    }
    state.initialized = true;
    std::ifstream input(path);
    if (!input) {
        result.diagnostics.push_back(path.u8string() + ": cannot open configuration; using all built-in defaults");
        return result;
    }
    auto parsed = parseBalance(input, path.u8string());
    result.diagnostics = std::move(parsed.diagnostics);
    if (!result.diagnostics.empty()) {
        result.diagnostics.push_back(path.u8string() + ": rejected configuration; using all built-in defaults");
        return result;
    }
    state.values = parsed.values;
    state.usingDefaults = false;
    result.loaded = true;
    result.usedDefaults = false;
    return result;
}

} // namespace br
