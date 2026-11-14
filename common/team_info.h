#pragma once

#include "ui.h"
#include <algorithm>
#include <string>

namespace br {

inline std::string teamCreditLine() {
    return "TEAM / Rukhanov Birzhan / Zhamenov Gani / Rinatuly Miras";
}

inline void drawTeamCredits(UI& ui,int width) {
    const std::string credits=teamCreditLine();
    const float scale=std::min(1.1f,float(width-24)/std::max(1.f,UI::textWidth(credits,1.f)));
    ui.rect(0,0,float(width),16,{.012f,.016f,.011f,.94f});
    ui.text((float(width)-UI::textWidth(credits,scale))*.5f,4,credits,scale,{.82f,.80f,.66f,1});
}

} // namespace br
