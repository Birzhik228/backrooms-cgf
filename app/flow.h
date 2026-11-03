#pragma once

namespace br::app {

// One presentation screen is active at a time. Encounter separately owns
// capture, blackout and game-over; a menu transition must not reset that state.
enum class Screen { Gameplay, Menu, Settings, Controls, LevelSelect, Ending };

class GameFlow {
public:
    // These describe the run, not competing screens. They survive ordinary
    // overlay changes (for example Settings -> Menu or Ending -> Menu).
    bool entered = false;
    bool guidePending = false;
    bool escaped = false;

    Screen screen() const { return screen_; }
    bool is(Screen value) const { return screen_ == value; }
    bool gameplay() const { return is(Screen::Gameplay); }
    bool cinematic() const { return is(Screen::Ending); }
    bool capturesInput() const { return !gameplay(); }

    void open(Screen value) { screen_ = value; }
    void resume() {
        screen_ = Screen::Gameplay;
        entered = true;
        escaped = false;
    }
    void backToMenu() { screen_ = Screen::Menu; }

private:
    Screen screen_ = Screen::Menu;
};

} // namespace br::app
