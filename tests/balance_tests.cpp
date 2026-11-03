#include "common/balance.h"
#include "common/gameplay.h"
#include "common/movement.h"
#include "common/player_state.h"
#include "common/threat.h"
#include "common/partygoer.h"
#include "common/darkness.h"

#include <chrono>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void near(double value, double expected, const char* message) {
    check(std::abs(value-expected) < .0001, message);
}

struct Fixture {
    std::filesystem::path path;
    explicit Fixture(const std::string& text) {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() /
               ("backrooms-balance-" + std::to_string(stamp) + ".cfg");
        std::ofstream out(path);
        out << text;
        if (!out) throw std::runtime_error("Cannot write temporary balance fixture");
    }
    ~Fixture() { std::error_code error; std::filesystem::remove(path, error); }
};

std::array<double, 37> values(const br::Balance& config) {
    return {{
        config.playerWalkSpeed, config.playerSprintSpeed, config.playerCrouchSpeed,
        config.playerAcceleration, config.playerBraking, config.staminaDrain,
        config.staminaRecovery, config.staminaDelay, config.staminaResumeThreshold,
        config.araneaWanderSpeed, config.araneaChaseSpeed, config.araneaEnragedMultiplier,
        config.araneaSpawnGrace, config.araneaSpawnMin, config.araneaSpawnMax,
        config.araneaSightRange, config.araneaMemory, config.araneaCatchDistance,
        config.araneaMotionThreshold, config.araneaHearingBase, config.araneaHearingPerSpeed,
        config.araneaHearingMax, config.araneaCrouchHearing, config.araneaDoorHearing,
        config.araneaWallHearing, config.araneaAlarmRange, config.araneaAlarmMemory,
        config.araneaWalkTurnRate, config.araneaChaseTurnRate, config.mouseApproachSpeed,
        config.mouseFleeSpeed, config.partygoerSpeed, config.partygoerAlarmSpeed,
        config.partygoerAlarmDuration, config.darknessChaseSpeed, config.darknessFlashlightRange,
        config.darknessCatchDistance,
    }};
}

br::BalanceParseResult parse(const std::string& text) {
    std::istringstream input(text);
    return br::parseBalance(input, "fixture.cfg");
}

void rejected(const std::string& text, const std::string& diagnostic) {
    const auto result = parse(text);
    check(!result.valid(), "Invalid configuration must be rejected");
    check(values(result.values) == values(br::Balance{}),
          "Invalid input cannot leak a partial configuration");
    bool found = false;
    for (const auto& message : result.diagnostics)
        found = found || message.find(diagnostic) != std::string::npos;
    check(found, "Validation should explain the offending setting/line");
}

void parserChecks() {
    std::ifstream shipped("balance.cfg");
    check(bool(shipped), "The shipped balance.cfg must be available");
    const auto defaults = br::parseBalance(shipped, "balance.cfg");
    check(defaults.valid() && values(defaults.values) == values(br::Balance{}),
          "Shipped configuration reproduces every compiled default exactly");
    check(br::balance().playerWalkSpeed == 2.5f && br::balance().playerSprintSpeed == 4.f &&
          br::balance().playerCrouchSpeed == 1.1f && br::balance().staminaDrain == 12.f &&
          br::balance().araneaWanderSpeed == 1.2f && br::balance().araneaChaseSpeed == 3.6f &&
          br::balance().araneaMemory == 5.f && br::balance().araneaSpawnGrace == 8.f,
          "Existing exploration and threat balance stays unchanged by default");
    const auto valid = parse("\xEF\xBB\xBF# UTF-8 BOM and CRLF\r\n player.walk_speed = 3 # note\r\n");
    check(valid.valid() && valid.values.playerWalkSpeed == 3 && valid.values.playerSprintSpeed == 4,
          "Partial overrides accept comments/BOM/whitespace and keep other defaults");
    near(br::balance().playerWalkSpeed, 2.5, "Parsing a proposal must not alter active gameplay");

    rejected("player.walk_speed=3\nunknown=2\n", "fixture.cfg:2: unknown key");
    rejected("player.walk_speed=3\nplayer.walk_speed=4\n", "fixture.cfg:2: duplicate key");
    rejected("player.walk_speed 3\n", "expected one key=value assignment");
    rejected("=3\n", "key and value must not be empty");
    rejected("player.walk_speed=\n", "key and value must not be empty");
    rejected("player.walk_speed=3=4\n", "expected one key=value assignment");
    rejected("player.walk_speed=3 trailing\n", "unexpected text after the value");
    for (const auto* text : {"nan", "inf", "-inf", "1e999", "garbage"})
        rejected(std::string("player.walk_speed=")+text+"\n", "requires a finite decimal number");
    rejected("player.walk_speed=0\n", "must be between");
    rejected("player.walk_speed=3\nplayer.sprint_speed=2\n", "fixture.cfg:2: player.sprint_speed");
    rejected("player.walk_speed=.5\nplayer.crouch_speed=1\n", "player.crouch_speed must not exceed");
    rejected("player.acceleration=20\nplayer.braking=10\n", "player.braking must be at least");
    rejected("aranea.wander_speed=4\naranea.chase_speed=3\n", "aranea.chase_speed must be at least");
    rejected("aranea.chase_speed=10\naranea.enraged_multiplier=2\n", "must not exceed 12 m/s");
    rejected("aranea.spawn_min_distance=32\naranea.spawn_max_distance=24\n", "spawn_max_distance must be at least");
    rejected("aranea.hearing_base=5\naranea.hearing_max=2\n", "hearing_max must be at least");
    rejected("aranea.walk_turn_rate=10\naranea.chase_turn_rate=5\n", "chase_turn_rate must be at least");
    rejected("mouse.approach_speed=3\nmouse.flee_speed=2\n", "mouse.flee_speed must be at least");
    rejected("partygoer.speed=8\npartygoer.alarm_speed=7\n", "partygoer.alarm_speed must be at least");
    rejected(std::string(4097, '#'), "configuration exceeds");
    rejected(std::string(257, '\n'), "configuration exceeds");
}

void alternateGameplay() {
    br::World world(42);
    world.update(3.75, 3.75);
    br::ThreatPlayer player; player.x=3.75; player.z=.9; player.speed=4; player.noise=1;
    br::Threat original;
    original.resetAt(3.75, 3.75, 0);
    original.update(world, player, .05f);
    check(original.state()==br::ThreatState::Chase, "Original senses detect the nearby player");
    near(original.speed(), 3.6, "Original chase speed is retained before an override");

    Fixture alternate(
        "player.walk_speed=3\nplayer.sprint_speed=6\nplayer.crouch_speed=.8\n"
        "player.acceleration=10\nplayer.braking=20\n"
        "stamina.drain_per_second=20\nstamina.recovery_per_second=10\n"
        "stamina.recovery_delay=.5\nstamina.resume_threshold=30\n"
        "aranea.wander_speed=.5\naranea.chase_speed=2\naranea.spawn_grace=12\n"
        "aranea.spawn_min_distance=16\naranea.spawn_max_distance=20\n"
        "aranea.sight_range=1\naranea.hearing_base=0\naranea.hearing_per_speed=0\n"
        "aranea.hearing_max=0\naranea.door_hearing_range=0\n"
        "partygoer.speed=4\npartygoer.alarm_speed=6\ndarkness.chase_speed=5\n");
    const auto loaded = br::loadBalance(alternate.path);
    check(loaded.loaded && !loaded.usedDefaults && loaded.diagnostics.empty(),
          "Validated file becomes the active run configuration");
    check(br::WALK_SPEED==3 && br::SPRINT_SPEED==6 && br::CROUCH_SPEED==.8f,
          "Existing movement call sites observe loaded speeds");
    check(br::Threat::SPAWN_MIN_DISTANCE==16 && br::Threat::SPAWN_MAX_DISTANCE==20 &&
          br::Partygoers::SPEED==4 && br::Partygoers::ALARM_SPEED==6 && br::Darkness::CHASE_SPEED==5,
          "Actor speed and spawn aliases observe loaded values across components");

    br::PlayerMotion motion;
    const auto displacement=motion.advance({br::WALK_SPEED,0,0}, .1f);
    near(motion.speed(), 1, "Configured acceleration changes real movement");
    near(displacement.x, .05, "Configured movement integrates distance correctly");
    br::Stamina stamina;
    stamina.update(2, true, false);
    near(stamina.value, 60, "Configured drain changes real stamina consumption");
    stamina.update(.5f, false, false);
    near(stamina.value, 60, "Configured recovery delay is honored");
    stamina.update(1, false, false);
    near(stamina.value, 70, "Configured recovery rate is honored");
    stamina.update(10, true, false);
    stamina.update(.5f, false, false);
    stamina.update(2, false, false);
    check(stamina.exhausted && stamina.value==20, "Configured resume threshold holds exhaustion");
    stamina.update(1, false, false);
    check(stamina.canSprint() && stamina.value==30, "Configured threshold restores sprint ability");

    br::Threat quiet;
    quiet.resetAt(3.75, 3.75, 0);
    quiet.update(world, player, .05f);
    check(quiet.state()==br::ThreatState::Wander,
          "Reduced sight and hearing ranges change actual enemy perception");
    quiet.resetAt(3.75, 3.75, 0);
    player.z=2.9;
    quiet.update(world, player, .05f);
    check(quiet.state()==br::ThreatState::Chase, "A player inside configured sight range is still detected");
    near(quiet.speed(), 2, "Configured chase speed changes navigation displacement");
    br::Threat dormant;
    for(int i=0;i<100;++i)dormant.update(world, player, .1f);
    check(!dormant.active(), "Configured grace period keeps initial exploration quiet");

    Fixture replacement("player.walk_speed=2\n");
    const auto refused=br::loadBalance(replacement.path);
    check(refused.alreadyInitialized && !refused.loaded && br::WALK_SPEED==3,
          "Configuration is frozen for the run; file changes require a restart");
}
} // namespace

int main(int argc, char** argv) {
    try {
        if(argc>1 && std::string(argv[1])=="--invalid-startup") {
            Fixture invalid("player.walk_speed=3\nunknown=1\n");
            const auto result=br::loadBalance(invalid.path);
            check(!result.loaded && result.usedDefaults && result.diagnostics.size()>=2,
                  "Invalid startup file should explain why defaults were selected");
            check(values(br::balance())==values(br::Balance{}),
                  "Invalid startup publishes no partial override");
        } else if(argc>1 && std::string(argv[1])=="--missing-startup") {
            Fixture missing(""); std::filesystem::remove(missing.path);
            const auto result=br::loadBalance(missing.path);
            check(!result.loaded && result.usedDefaults && !result.diagnostics.empty() &&
                  values(br::balance())==values(br::Balance{}),
                  "Missing configuration starts safely with original defaults");
        } else {
            parserChecks();
            alternateGameplay();
        }
        std::cout << "Balance configuration tests passed.\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
