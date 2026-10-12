#version 330 core
in vec2 uv;
out vec4 frag;
uniform sampler2D uScene;
uniform sampler2D uAO,uDepth;
uniform float uAOAmount;
uniform mat4 uProjection;
float depthAtPixel(ivec2 pixel){
    ivec2 size=textureSize(uDepth,0);
    float z=texelFetch(uDepth,clamp(pixel,ivec2(0),size-1),0).r*2.0-1.0;
    return uProjection[3][2]/(z+uProjection[2][2]);
}
float contactAO(){
    float center=depthAtPixel(ivec2(gl_FragCoord.xy)),sum=0,weight=0;
    ivec2 depthSize=textureSize(uDepth,0),size=textureSize(uAO,0);
    // Match the representative depth pixel (2*i+1) used by the AO pass.
    // Bilinear spatial weights avoid stepping between coarse AO texels.
    vec2 grid=(gl_FragCoord.xy-vec2(1.5))*.5;
    // Odd-sized buffers end with a one-pixel interval after depth clamping.
    if((depthSize.x&1)==1&&int(gl_FragCoord.x)==depthSize.x-1)grid.x=float(size.x-1);
    if((depthSize.y&1)==1&&int(gl_FragCoord.y)==depthSize.y-1)grid.y=float(size.y-1);
    ivec2 base=ivec2(floor(grid));vec2 fraction=fract(grid);
    for(int y=0;y<2;y++)for(int x=0;x<2;x++){
        ivec2 pixel=clamp(base+ivec2(x,y),ivec2(0),size-1);
        ivec2 depthPixel=min(pixel*2+ivec2(1),depthSize-1);
        float spatial=(x==0?1.0-fraction.x:fraction.x)*(y==0?1.0-fraction.y:fraction.y);
        float w=spatial*exp(-abs(depthAtPixel(depthPixel)-center)*24.0);
        sum+=texelFetch(uAO,pixel,0).r*w;weight+=w;
    }
    return weight>.0001?sum/weight:1.0;
}
uniform float uTime;
uniform vec3 uSize;
uniform float uBlurAmount;
uniform float uFear;
uniform float uScareTime, uChromatic, uStatic, uVignette, uRed, uFlash, uBlack, uGlow;
float cameraNoise(uvec2 pixel, uint frame, uint salt) {
    // Integer mixing keeps adjacent pixels independent at every screen size
    // and session length; large sine arguments produced visible column bands.
    uint h = pixel.x * 0x9e3779b9u ^ pixel.y * 0x85ebca6bu
           ^ frame * 0xc2b2ae35u ^ salt;
    h ^= h >> 16u;
    h *= 0x7feb352du;
    h ^= h >> 15u;
    h *= 0x846ca68bu;
    h ^= h >> 16u;
    return float(h >> 8u) * (1.0 / 16777216.0) - 0.5;
}
void main() {
    vec2 p = uv - 0.5;
    float edge = dot(p, p);
    // A compact 13-tap, two-ring kernel gives a lightly softened camcorder
    // image. It is still one fullscreen pass, with no extra buffers.
    // Scale against the reference viewport so high DPI does not erase the
    // effect. The UI is rendered afterward and remains readable.
    vec2 texel = 1.0 / max(uSize.xy, vec2(1.0));
    float referenceScale = clamp(uSize.y / 800.0, 0.75, 1.5);
    vec2 radius = texel * (0.8 + 0.3 * edge) * referenceScale * clamp(uBlurAmount, 0.0, 1.5);
    vec2 inner = radius * 0.5;
    vec2 diagonal = radius * 0.70710678;
    vec3 color = texture(uScene, uv).rgb * 0.12;
    color += texture(uScene, uv + vec2(inner.x, 0.0)).rgb * 0.11;
    color += texture(uScene, uv - vec2(inner.x, 0.0)).rgb * 0.11;
    color += texture(uScene, uv + vec2(0.0, inner.y)).rgb * 0.11;
    color += texture(uScene, uv - vec2(0.0, inner.y)).rgb * 0.11;
    color += texture(uScene, uv + vec2(radius.x, 0.0)).rgb * 0.04;
    color += texture(uScene, uv - vec2(radius.x, 0.0)).rgb * 0.04;
    color += texture(uScene, uv + vec2(0.0, radius.y)).rgb * 0.04;
    color += texture(uScene, uv - vec2(0.0, radius.y)).rgb * 0.04;
    color += texture(uScene, uv + diagonal).rgb * 0.07;
    color += texture(uScene, uv - diagonal).rgb * 0.07;
    color += texture(uScene, uv + vec2(diagonal.x, -diagonal.y)).rgb * 0.07;
    color += texture(uScene, uv + vec2(-diagonal.x, diagonal.y)).rgb * 0.07;

    if(uAOAmount>0.0)color*=mix(1.0,contactAO(),clamp(uAOAmount,0.0,1.0));
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luminance), color, 0.92);
    // Slightly green shadows and warm highlights suggest an old camcorder's
    // imperfect fluorescent white balance, without obscuring gameplay.
    vec3 shadowTint = vec3(0.966, 0.990, 0.961);
    vec3 lightTint = vec3(1.025, 1.010, 0.966);
    color *= mix(shadowTint, lightTint, smoothstep(0.20, 0.80, luminance));
    color = (color - 0.5) * 1.045 + 0.5;
    color *= 1.0 - 0.42 * smoothstep(0.08, 0.51, edge);

    // Bound the float-to-uint conversion while preserving the existing 24 Hz cadence.
    uint frame = uint(mod(floor(max(uTime, 0.0) * 24.0), 16777216.0));
    float grain = cameraNoise(uvec2(gl_FragCoord.xy), frame, 0x68bc21ebu);
    color += grain * mix(0.021, 0.010, luminance);
    float fear=clamp(uFear,0.0,1.0);
    color=mix(color,vec3(dot(color,vec3(.2126,.7152,.0722))),fear*.36);
    color*=1.0-fear*.45*smoothstep(.04,.45,edge);
    color+=grain*fear*.045;
    // Scare-only bright-pass blur: additive halo from the already rendered
    // emissive teeth/eyes. No persistent FBO or resize-dependent blur history.
    if(uGlow>0.0) {
        vec3 halo=vec3(0);
        for(int i=0;i<12;++i) {
            float a=float(i)*6.2831853/12.0;
            vec2 offset=vec2(cos(a),sin(a))*texel*mix(5.0,13.0,float(i%2));
            vec3 sampleColor=texture(uScene,clamp(uv+offset,vec2(0),vec2(1))).rgb;
            halo+=max(sampleColor-vec3(.82),vec3(0));
        }
        color+=halo*(uGlow*.32);
    }
    if(uChromatic>0.0) {
        vec2 split=p*uChromatic*(1.0+.35*sin(uScareTime*47.0));
        color.r+=texture(uScene,clamp(uv+split,vec2(0),vec2(1))).r-texture(uScene,uv).r;
        color.b+=texture(uScene,clamp(uv-split,vec2(0),vec2(1))).b-texture(uScene,uv).b;
    }
    uint staticFrame = uint(mod(floor(max(uScareTime, 0.0) * 90.0), 16777216.0));
    float staticNoise = cameraNoise(uvec2(gl_FragCoord.xy), staticFrame, 0x02e5be93u);
    color+=staticNoise*uStatic;
    color*=1.0-uVignette*(.8+.2*sin(uScareTime*31.0))*smoothstep(.03,.4,edge);
    color=mix(color,color*vec3(1.3,.12,.08)+vec3(.08,0,0),uRed);
    color=mix(color,vec3(1),uFlash);
    color*=1.0-uBlack;
    frag = vec4(clamp(color, 0.0, 1.0), 1.0);
}
