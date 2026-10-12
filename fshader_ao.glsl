#version 330 core
in vec2 uv;
out float occlusion;
uniform sampler2D uDepth;
uniform mat4 uProjection;
uniform vec3 uSize;
vec3 positionAtPixel(ivec2 pixel){
    ivec2 size=textureSize(uDepth,0);
    pixel=clamp(pixel,ivec2(0),size-1);
    vec2 p=(vec2(pixel)+.5)/vec2(size);
    float z=texelFetch(uDepth,pixel,0).r*2.0-1.0;
    float depth=uProjection[3][2]/(z+uProjection[2][2]);
    return vec3((p*2.0-1.0)*depth/vec2(uProjection[0][0],uProjection[1][1]),-depth);
}
vec3 position(vec2 p){
    return positionAtPixel(ivec2(floor(p*vec2(textureSize(uDepth,0)))));
}
void main(){
    // A half-resolution pixel owns one exact full-resolution depth sample.
    // Sampling at the boundary between depth texels produced false normals
    // and repeated dark bands on walls seen at a grazing angle.
    ivec2 size=textureSize(uDepth,0);
    ivec2 pixel=min(ivec2(gl_FragCoord.xy)*2+ivec2(1),size-1);
    if(texelFetch(uDepth,pixel,0).r>=.99999){occlusion=1;return;}
    vec3 p=positionAtPixel(pixel);
    // Choose the nearer depth neighbour at silhouettes rather than deriving
    // a false normal across the far background.
    vec3 l=positionAtPixel(pixel-ivec2(1,0))-p;
    vec3 r=positionAtPixel(pixel+ivec2(1,0))-p;
    vec3 b=positionAtPixel(pixel-ivec2(0,1))-p;
    vec3 a=positionAtPixel(pixel+ivec2(0,1))-p;
    vec3 dx=pixel.x==0?r:pixel.x==size.x-1?-l:(abs(l.z)<abs(r.z)?-l:r);
    vec3 dy=pixel.y==0?a:pixel.y==size.y-1?-b:(abs(b.z)<abs(a.z)?-b:a);
    vec3 n=cross(dx,dy);
    if(dot(n,n)<1e-16){occlusion=1;return;}
    n=normalize(n);
    if(dot(n,-p)<0)n=-n;
    vec3 tangent=normalize(cross(n,abs(n.y)<.9?vec3(0,1,0):vec3(1,0,0)));
    vec3 bitangent=cross(n,tangent);
    float blocked=0;
    for(int i=0;i<12;i++){
        float angle=float(i)*2.39996323;
        float h=.25+.65*float(i%4)/3.0;
        vec3 dir=tangent*cos(angle)*sqrt(1-h*h)+bitangent*sin(angle)*sqrt(1-h*h)+n*h;
        vec3 q=p+dir*(.10+.36*float(i+1)/12.0);
        vec4 clip=uProjection*vec4(q,1);
        if(clip.w<=0.0)continue;
        vec2 at=clip.xy/clip.w*.5+.5;
        if(any(lessThan(at,vec2(0)))||any(greaterThan(at,vec2(1))))continue;
        vec3 hit=position(at);
        float range=1.0-smoothstep(.25,.65,abs(hit.z-p.z));
        // A depth texel can be closer along Z while still lying on this wall.
        // Only geometry above the receiver plane can occlude its hemisphere.
        blocked+=(hit.z>q.z+.018&&dot(hit-p,n)>.012?1.0:0.0)*range;
    }
    occlusion=clamp(1.0-blocked/12.0*.65,.45,1.0);
}
