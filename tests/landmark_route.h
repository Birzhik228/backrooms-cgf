#pragma once
#include "world.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
inline std::vector<br::WorldPoint> landmarkRoute(std::uint64_t seed) {
    auto cell=br::exitCell(seed);std::vector<br::WorldPoint> points;
    for(int guard=0;guard<100;++guard) {
        points.push_back({(cell.x+.5)*br::CELL,(cell.z+.5)*br::CELL});
        if(cell.x==0 && cell.z==0)break;
        bool found=false;
        for(auto d:{br::ChunkCoord{1,0},{-1,0},{0,1},{0,-1}}) {
            br::ChunkCoord n{cell.x+d.x,cell.z+d.z};
            if(std::abs(n.x)+std::abs(n.z)>=std::abs(cell.x)+std::abs(cell.z))continue;
            if(br::edgeReserved(cell.x,cell.z,n.x,n.z,seed)){cell=n;found=true;break;}
        }
        if(!found)throw std::runtime_error("missing objective route");
    }
    std::reverse(points.begin(),points.end());points.push_back(br::World(seed).exitLocation());return points;
}
