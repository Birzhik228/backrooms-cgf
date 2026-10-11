#version 330 core
in vec2 uv;
out vec4 frag;
uniform sampler2D uScene;
uniform float uTime;
uniform vec3 uSize;
uniform float uBlurAmount;
uniform float uFear;
uniform float uScareTime, uChromatic, uStatic, uVignette, uRed, uFlash, uBlack, uGlow;
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

    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luminance), color, 0.92);
    // Slightly green shadows and warm highlights suggest an old camcorder's
    // imperfect fluorescent white balance, without obscuring gameplay.
    vec3 shadowTint = vec3(0.966, 0.990, 0.961);
    vec3 lightTint = vec3(1.025, 1.010, 0.966);
    color *= mix(shadowTint, lightTint, smoothstep(0.20, 0.80, luminance));
    color = (color - 0.5) * 1.045 + 0.5;
    color *= 1.0 - 0.42 * smoothstep(0.08, 0.51, edge);

    float frame = floor(uTime * 24.0);
    float grain = fract(sin(dot(gl_FragCoord.xy + frame * vec2(17.0, 31.0),
                               vec2(12.9898, 78.233))) * 43758.5453) - 0.5;
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
    float staticNoise=fract(sin(dot(gl_FragCoord.xy+floor(uScareTime*90.0),vec2(12.9898,78.233)))*43758.5453)-.5;
    color+=staticNoise*uStatic;
    color*=1.0-uVignette*(.8+.2*sin(uScareTime*31.0))*smoothstep(.03,.4,edge);
    color=mix(color,color*vec3(1.3,.12,.08)+vec3(.08,0,0),uRed);
    color=mix(color,vec3(1),uFlash);
    color*=1.0-uBlack;
    frag = vec4(clamp(color, 0.0, 1.0), 1.0);
}
