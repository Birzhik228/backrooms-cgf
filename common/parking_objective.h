#pragma once
#include "../world.h"
#include <cmath>

namespace br {
inline int parkingCircuitTarget(const World& world,double x,double z) {
    if(!world.isParking() || !std::isfinite(x) || !std::isfinite(z))return -1;
    int target=-1;double best=1e30;
    for(int i=0;i<3;++i)if(!world.breakerPowered(i)) {
        const auto p=world.breakerLocation(i);
        const double distance=std::hypot(p.x-x,p.z-z);
        if(distance<best){best=distance;target=i;}
    }
    return target;
}
}
