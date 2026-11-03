#include "app/flow.h"
#include "common/encounter.h"

#include <iostream>
#include <stdexcept>

using br::app::GameFlow;
using br::app::Screen;

static void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    GameFlow flow;
    flow.guidePending = true;
    check(flow.is(Screen::Menu) && flow.capturesInput() && !flow.entered,
          "Startup shows the menu without entering a run");
    flow.open(Screen::Settings);
    flow.backToMenu();
    check(!flow.entered && flow.guidePending && flow.is(Screen::Menu),
          "Visiting startup settings preserves the pending first-entry guide");

    flow.resume();
    flow.guidePending = false; // The controller consumes the pending guide once.
    flow.open(Screen::Controls);
    check(flow.entered && flow.capturesInput() && !flow.gameplay(),
          "The controls guide suspends an entered run");
    flow.resume();
    check(flow.gameplay() && !flow.capturesInput() && !flow.guidePending,
          "Dismissing controls resumes play without scheduling the guide again");

    flow.backToMenu();
    flow.open(Screen::Settings);
    flow.open(Screen::LevelSelect);
    check(flow.screen() == Screen::LevelSelect && !flow.is(Screen::Settings),
          "Opening the level selector replaces the previous overlay");
    flow.resume(); // The controller has installed the selected level.
    check(flow.entered && flow.gameplay() && !flow.escaped,
          "Entering a selected level starts playable exploration");

    br::Encounter encounter;
    check(encounter.catchPlayer(), "The encounter begins capture");
    flow.backToMenu();
    flow.open(Screen::Settings);
    flow.backToMenu();
    check(encounter.scaring() && flow.entered && flow.capturesInput(),
          "Changing overlays cannot clear capture or the run's entry history");
    encounter.reset(); // Restart owns the gameplay reset, not screen selection.
    flow.resume();
    check(encounter.alive() && flow.gameplay(),
          "An explicitly reset encounter can resume at its checkpoint");

    flow.escaped = true;
    flow.open(Screen::Ending);
    check(flow.cinematic() && flow.capturesInput() && flow.escaped,
          "The ending captures input while retaining completion");
    flow.backToMenu();
    check(!flow.cinematic() && flow.entered && flow.escaped,
          "Leaving the ending preserves the completed-run menu context");
    flow.open(Screen::Settings);
    flow.backToMenu();
    check(flow.escaped, "Settings do not erase a completed run");
    flow.resume();
    check(flow.gameplay() && !flow.escaped,
          "Continuing exploration clears the completion presentation");

    std::cout << "Game flow transition tests passed.\n";
}
