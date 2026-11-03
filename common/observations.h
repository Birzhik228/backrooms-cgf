#pragma once
#include "ui.h"
#include <algorithm>
#include <array>

namespace br {
struct ObservationText { const char* title; const char* first; const char* second; };
inline ObservationText observationText(int kind) {
    constexpr std::array<ObservationText,3> notes={{
        {"MAINTENANCE NOTE", "REPLACED BOTH TUBES. THE SCRAPING CAME BACK.",
         "IT STOPPED WHEN I STOPPED WALKING."},
        {"CEILING INSPECTION", "THE PANELS WERE BROKEN FROM BELOW.",
         "I COULD HEAR IT MOVING IN THE NEXT ROOM."},
        {"TORN NOTE", "IF IT SEES YOU, BREAK SIGHT BEFORE HIDING.",
         "STAY QUIET. LISTEN BEFORE OPENING A DOOR."}
    }};
    return notes[std::clamp(kind,0,2)];
}

inline void drawObservation(UI& ui,int width,int height,int kind) {
    const float scale=std::min(1.1f,(width-40)/760.f);
    const float left=(width-760*scale)*.5f,top=height*.56f;
    const Color paper{.89f,.87f,.73f,1},gold{.78f,.66f,.36f,1};
    const auto note=observationText(kind);
    ui.rect(left,top,760*scale,108*scale,{.035f,.040f,.024f,.94f});
    ui.rect(left,top,3*scale,108*scale,gold);
    ui.text(left+22*scale,top+16*scale,note.title,1.45f*scale,gold);
    ui.text(left+22*scale,top+46*scale,note.first,1.65f*scale,paper);
    ui.text(left+22*scale,top+74*scale,note.second,1.65f*scale,paper);
}
}
