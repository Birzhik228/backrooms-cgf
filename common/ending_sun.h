#pragma once
#include "math.h"
#include <array>
#include <limits>

namespace br {
// Fixed directional cascades for the finite city film. Their projections never
// follow the camera, so stationary building shadows cannot shimmer while panning.
// Static and animated casters share each cascade's light-space coordinates.
class EndingSun {
    static Mat4 buildMatrix(int cascade) {
        const Vec3 low=cascade==0?Vec3{-32,-1,-42}:Vec3{-112,-1,-85};
        const Vec3 high=cascade==0?Vec3{48,28,0}:Vec3{112,55,5};
        const Vec3 center=(low+high)*.5f,sun=direction();
        const Mat4 view=lookAt(center+sun*220.f,sun*-1.f);
        float left=std::numeric_limits<float>::max(),right=-left;
        float bottom=left,top=-left;
        for(int corner=0;corner<8;++corner) {
            const Vec3 p{corner&1?high.x:low.x,corner&2?high.y:low.y,corner&4?high.z:low.z};
            const float x=view.m[0]*p.x+view.m[4]*p.y+view.m[8]*p.z+view.m[12];
            const float y=view.m[1]*p.x+view.m[5]*p.y+view.m[9]*p.z+view.m[13];
            left=std::min(left,x);right=std::max(right,x);
            bottom=std::min(bottom,y);top=std::max(top,y);
        }
        left-=2;right+=2;bottom-=2;top+=2;
        // Keep the depth range wide enough for off-camera buildings that cast
        // into a receiver cascade. Only receiver XY, never caster XY/depth, is
        // tightened; every city caster is drawn and its raster fragments clip.
        constexpr float nearPlane=1,farPlane=500;
        Mat4 projection=Mat4::identity();
        projection.m[0]=2/(right-left);projection.m[5]=2/(top-bottom);
        projection.m[10]=-2/(farPlane-nearPlane);
        projection.m[12]=-(right+left)/(right-left);
        projection.m[13]=-(top+bottom)/(top-bottom);
        projection.m[14]=-(farPlane+nearPlane)/(farPlane-nearPlane);
        return projection*view;
    }
public:
    // Toward the sun: visible above the roofs during the final rightward look,
    // with shadows falling diagonally across the road toward -X/+Z.
    static Vec3 direction() {
        static const Vec3 sun=normalize(Vec3{.85f,.38f,-.18f});
        return sun;
    }
    static Mat4 matrix(int cascade) {
        static const std::array<Mat4,2> matrices={buildMatrix(0),buildMatrix(1)};
        return matrices[cascade==0?0:1];
    }
    static constexpr int staticResolution(int) {return 2048;}
    static constexpr int dynamicResolution(int cascade) {return cascade==0?2048:1024;}
};
}
