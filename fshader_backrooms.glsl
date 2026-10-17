#version 330 core

in vec3 vPosition;
in vec3 vNormal;
in vec2 vUV;
flat in float vMaterial;

out vec4 fragColor;

uniform vec3 uCamera;
uniform vec3 uForward;
uniform vec3 uFlashPosition;
uniform mat4 uFlashVP;
uniform bool uFlashShadows;
uniform sampler2DShadow uFlashShadow;
uniform sampler2DArrayShadow uLampShadows;
uniform vec3 uWorldPhase;
uniform float uTime;
uniform float uExposure;
uniform float uScareDarken;
uniform float uScareGlow;
uniform bool uFlashlight;
uniform bool uClues;
uniform bool uFog;
uniform float uRenderDistance;
uniform vec3 uLights[24];
uniform float uLightPower[24];
uniform vec3 uLightColor[24];
uniform sampler2D uWallpaper;
uniform sampler2D uCarpet;
uniform sampler2D uCeiling;
uniform sampler2D uWallpaperNormal;
uniform sampler2D uCarpetNormal;
uniform sampler2D uCeilingNormal;
uniform sampler2D uHardFloor;
uniform sampler2D uHardFloorNormal;
uniform sampler2D uCreatureColor;
uniform sampler2D uCreatureNormal;

// Color images and room material RGB values are display-space colors. The
// creature's imported glTF factors are already linear. All illumination is
// linear, with a single tone map and gamma conversion below.
vec3 linearColor(vec3 c) { return pow(max(c, vec3(0.0)), vec3(2.2)); }

float lampEnvelope(int state, float phase) {
    // Identical to the CPU helper in world.h. Each fixture carries its own
    // seeded phase rather than the index of a changing nearest-light list.
    if (state == 2) return 0.0;
    float p = phase * 6.28318530718;
    if (state == 1) {
        return (0.12 + 0.50 * smoothstep(-0.2, 0.45, sin(uTime * 2.7 + p)))
             * (0.94 + 0.06 * sin(uTime * 11.0 + p * 2.3));
    }
    return 0.98 + 0.014 * sin(uTime * 3.1 + p) + 0.006 * sin(uTime * 7.7 + p * 2.3);
}

vec3 mappedNormal(vec3 geometricNormal, vec2 coordinates, vec3 encoded, float strength) {
    // Recover +U/+V from the actual surface and texture derivatives. This
    // handles opposite-facing walls, downward ceilings and mirrored floor
    // UVs without extra per-vertex tangents or an incorrect global basis.
    vec3 dpdx = dFdx(vPosition), dpdy = dFdy(vPosition);
    vec2 duvdx = dFdx(coordinates), duvdy = dFdy(coordinates);
    float determinant = duvdx.x * duvdy.y - duvdx.y * duvdy.x;
    if (abs(determinant) < 1e-12) return geometricNormal;
    vec3 tangent = (dpdx * duvdy.y - dpdy * duvdx.y) / determinant;
    vec3 bitangent = (dpdy * duvdx.x - dpdx * duvdy.x) / determinant;
    tangent -= geometricNormal * dot(tangent, geometricNormal);
    if (dot(tangent, tangent) < 1e-12) return geometricNormal;
    tangent = normalize(tangent);
    bitangent -= geometricNormal * dot(bitangent, geometricNormal)
               + tangent * dot(bitangent, tangent);
    if (dot(bitangent, bitangent) < 1e-12) return geometricNormal;
    bitangent = normalize(bitangent);
    vec3 detail = encoded * 2.0 - 1.0;
    detail.xy *= strength;
    return normalize(tangent * detail.x + bitangent * detail.y
                   + geometricNormal * max(detail.z, 0.01));
}

float grain(vec2 uv) {
    return fract(sin(dot(floor(uv * 160.0), vec2(12.9898, 78.233))) * 43758.5453);
}

// A tiny original 5x7 bitmap alphabet spells EXIT on the normalized sign face.
// Bit zero is the rightmost column; UV y increases from bottom to top.
bool exitLetter(int letter, ivec2 pixel) {
    if (pixel.x < 0 || pixel.x >= 5 || pixel.y < 0 || pixel.y >= 7) return false;
    int row = 0;
    if (letter == 0) { // E
        row = (pixel.y == 0 || pixel.y == 6) ? 31 : (pixel.y == 3 ? 30 : 16);
    } else if (letter == 1) { // X
        row = (pixel.y == 0 || pixel.y == 6) ? 17 : ((pixel.y == 1 || pixel.y == 5) ? 10 : 4);
    } else if (letter == 2) { // I
        row = (pixel.y == 0 || pixel.y == 6) ? 31 : 4;
    } else if (letter == 3) { // T
        row = pixel.y == 6 ? 31 : 4;
    }
    return (row & (1 << (4 - pixel.x))) != 0;
}

float exitText(vec2 uv) {
    vec2 p = (uv - vec2(0.07, 0.20)) / vec2(0.86, 0.60);
    if (p.x < 0.0 || p.x >= 1.0 || p.y < 0.0 || p.y >= 1.0) return 0.0;
    ivec2 pixel = ivec2(floor(p * vec2(23.0, 7.0)));
    return exitLetter(pixel.x / 6, ivec2(pixel.x % 6, pixel.y)) ? 1.0 : 0.0;
}

float flashVisibility(vec3 normal) {
    if(!uFlashShadows)return 1.0;
    vec4 clip=uFlashVP*vec4(vPosition+normal*.008,1);
    if(clip.w<=0.0)return 0.0;
    vec3 p=clip.xyz/clip.w*.5+.5;
    if(any(lessThan(p,vec3(0)))||any(greaterThan(p,vec3(1))))return 0.0;
    float visibility=0.0;
    vec2 texel=1.0/vec2(textureSize(uFlashShadow,0));
    for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++)
        visibility+=texture(uFlashShadow,vec3(p.xy+vec2(x,y)*texel,p.z-.00015));
    return visibility/9.0;
}
float lampVisibility(int index,vec3 normal) {
    vec3 delta=vPosition+normal*.018-uLights[index];
    vec3 a=abs(delta);vec3 f;vec3 up=vec3(0,1,0);int face;
    if(a.x>=a.y&&a.x>=a.z){face=delta.x>0?0:1;f=vec3(sign(delta.x),0,0);}
    else if(a.y>=a.z){face=delta.y>0?2:3;f=vec3(0,sign(delta.y),0);up=vec3(0,0,1);}
    else {face=delta.z>0?4:5;f=vec3(0,0,sign(delta.z));}
    vec3 s=normalize(cross(f,up));vec3 u=cross(s,f);
    float distance=dot(delta,f);
    if(distance<.05)return 1.0;
    vec2 at=vec2(dot(delta,s),dot(delta,u))/distance*.5+.5;
    // Exact same .05..10 perspective depth as the six lamp render views.
    float depth=10.0/9.95-(10.0*.05)/(9.95*distance);
    return texture(uLampShadows,vec4(at,float(index*6+face),depth-.0003));
}
float wearHash(vec2 cell){return fract(sin(dot(mod(cell,36.0),vec2(127.1,311.7)))*43758.5453);}
float wearNoise(vec2 p) {
    vec2 cell=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    float a=wearHash(cell);
    float b=wearHash(cell+vec2(1,0));
    float c=wearHash(cell+vec2(0,1));
    float d=wearHash(cell+vec2(1,1));
    return mix(mix(a,b,f.x),mix(c,d,f.x),f.y);
}
void main() {
    // Integer material IDs remain unchanged; fluorescent tubes additionally
    // encode their stable phase in the fractional part of IDs 32, 33 and 34.
    int material = int(floor(vMaterial));
    if(material==57) {
        // This intentionally light-absorbing exit lining has no useful specular
        // response. Avoid all 24 lamps and flashlight PCF on its large surface.
        float shade=(.008+.003*grain(vUV*3.0))*uExposure;
        float fade=uFog?1.0-smoothstep(max(uRenderDistance,10.0)*.62,max(uRenderDistance,10.0)-2.0,length(uCamera-vPosition)):1.0;
        fragColor=vec4(vec3(shade*fade),1);return;
    }
    vec3 geometricNormal = normalize(vNormal);
    vec3 normal = geometricNormal;
    vec3 toEye = uCamera - vPosition;
    float distanceToEye = length(toEye);
    vec3 viewDirection = toEye / max(distanceToEye, 0.0001);
    vec3 flashDelta=uFlashPosition-vPosition;
    float flashDistance=length(flashDelta);
    vec3 flashDirection=flashDelta/max(flashDistance,.0001);
    float cone = smoothstep(0.84, 0.955, dot(-flashDirection, normalize(uForward)));
    float flashlight = uFlashlight ? cone / (1.0 + flashDistance * flashDistance * 0.115) : 0.0;
    if(flashlight>0.0001)flashlight*=flashVisibility(geometricNormal);

    vec3 albedo = linearColor(vec3(0.50));
    vec3 emission = vec3(0.0);
    float specularStrength = 0.035;
    float shininess = 20.0;
    if (material == 40) {
        float wear=wearNoise(vUV*2.0);
        albedo=linearColor(mix(vec3(.66,.225,.055),vec3(.86,.36,.10),wear*.65));
        specularStrength=.018;
    } else if (material == 41) {
        float fibers=texture(uCarpet,vUV).r;
        albedo=linearColor(vec3(.075,.29,.065)*(.70+.65*fibers));
        normal=mappedNormal(geometricNormal,vUV,texture(uCarpetNormal,vUV).rgb,.65);
        specularStrength=.008;
    } else if (material == 42) {
        vec2 cell=floor(vUV*8.0),at=fract(vUV*8.0)-.5;
        float roll=wearHash(cell);
        vec3 confetti=roll<.33?vec3(.76,.12,.12):roll<.66?vec3(.12,.38,.65):vec3(.24,.56,.13);
        float dotMask=(1.0-smoothstep(.07,.105,length(at)))*step(.60,roll);
        albedo=linearColor(mix(vec3(.87,.85,.73),confetti,dotMask))*(.97+.03*grain(vUV));
        specularStrength=.015;
    } else if (material == 43) {
        albedo=linearColor(vec3(.07,.23,.53))*(.94+.06*grain(vUV));
        specularStrength=.16;shininess=42;
    } else if (material == 44) {
        albedo=linearColor(vec3(.67,.39,.16))*(.87+.13*grain(vUV));
        specularStrength=.01;
    } else if (material == 45 || material == 46) {
        albedo=linearColor(material==45?vec3(.94,.93,.84):vec3(.92,.78,.65));
        specularStrength=material==45?.24:.04;shininess=60;
    } else if (material >= 48 && material <= 53) {
        const vec3 colors[6]=vec3[6](vec3(.82,.045,.065),vec3(.95,.67,.08),vec3(.045,.44,.23),
            vec3(.07,.34,.73),vec3(.48,.12,.58),vec3(.96,.32,.06));
        albedo=linearColor(colors[material-48]);specularStrength=.42;shininess=85;
    } else if (material == 57) {
        albedo=linearColor(vec3(.018,.020,.019))*(.8+.2*grain(vUV*3.0));
        specularStrength=0.0;
    } else if (material == 55 || material == 56) {
        float grime=wearNoise(vUV*9.0+vec2(5,17));
        vec3 yellow=vec3(.82,.67,.13)*(.91+.09*grime);
        yellow*=.94+.06*grain(vUV*5.0);
        if(material==56) {
            vec2 p=vUV;
            float eyes=max(1.0-smoothstep(.85,1.0,length((p-vec2(.30,.68))/vec2(.065,.15))),
                           1.0-smoothstep(.85,1.0,length((p-vec2(.70,.68))/vec2(.065,.15))));
            float curve=.22+.65*(p.x-.5)*(p.x-.5);
            float smile=(1.0-smoothstep(.045,.065,abs(p.y-curve)))*smoothstep(.12,.17,p.x)*(1.0-smoothstep(.83,.88,p.x));
            yellow=mix(yellow,vec3(.40,.045,.025),max(eyes,smile));
        } else {
            float stain=smoothstep(.66,.82,wearNoise(vUV*1.6+vec2(31,4)));
            yellow=mix(yellow,vec3(.34,.16,.035),stain*.16);
        }
        albedo=linearColor(yellow);specularStrength=.025;shininess=14;
    } else if (material == 0) {
        // A 12 m-wide sheet contains the original floral paper and baked wear.
        // The CPU supplies the render origin modulo 180 m, a common multiple
        // of the 12 m sheet and the 45 m chunk. Adding that bounded phase keeps
        // scars fixed across origin shifts, even at distant world coordinates.
        // Each of the six 7.5 m wall planes shifts the paper by 2 m. Mapping
        // from position also joins the separate faces around a doorway.
        bool eastWall = abs(geometricNormal.x) > 0.5;
        vec3 paperPosition = vPosition + uWorldPhase;
        float alongWall = eastWall ? paperPosition.z : paperPosition.x;
        float wallPlane = eastWall ? paperPosition.x : paperPosition.z;
        float planePhase = floor(mod(wallPlane + 0.20, 45.0) / 7.5) / 6.0;
        vec2 wallpaperUV = vec2(alongWall / 12.0 + planePhase, vPosition.y / 3.2);
        albedo = linearColor(texture(uWallpaper, wallpaperUV).rgb);
        normal = mappedNormal(geometricNormal, wallpaperUV,
                              texture(uWallpaperNormal, wallpaperUV).rgb, 1.0);
        // Restrained extra wear uses the same bounded world phase as the paper.
        float age=wearNoise(vec2(alongWall*.2+planePhase*9.0,vPosition.y*.7));
        albedo*=mix(.93,1.03,age);
        float ripple=sin(alongWall*4.188790205+age*4.0)*.025;
        normal=normalize(normal+vec3(eastWall?0.0:ripple,ripple*.25,eastWall?ripple:0.0));
        // Damp staining is material color, not a simulated contact shadow.
        float dampHeight = 0.25 + 0.10 * sin(wallpaperUV.x * 12.566371)
                                  + 0.035 * sin(wallpaperUV.x * 56.548668);
        albedo *= mix(0.66, 1.0, smoothstep(0.04, dampHeight + 0.28, vPosition.y));
    } else if (material == 1 || material == 18) {
        albedo = linearColor(texture(uCarpet, vUV).rgb);
        bool wet = material == 18;
        normal = mappedNormal(geometricNormal, vUV, texture(uCarpetNormal, vUV).rgb,
                              wet ? 0.45 : 1.0);
        albedo *= wet ? vec3(0.58, 0.63, 0.65) : vec3(1.0);
        specularStrength = wet ? 0.26 : 0.003;
        shininess = wet ? 48.0 : 8.0;
        vec2 floorPosition=(vPosition.xz+uWorldPhase.xz);
        float wear=wearNoise(floorPosition*.4);
        albedo*=mix(.92,1.015,wear);
        normal=normalize(mix(normal,geometricNormal,(1.0-smoothstep(.25,.65,wear))*.24));
        float wetPatch=smoothstep(.78,.92,wearNoise(floorPosition*.2+vec2(31,7)));
        specularStrength=mix(specularStrength,wet?.32:.09,wetPatch);
        shininess=mix(shininess,48.0,wetPatch);
    } else if (material == 2) {
        albedo = linearColor(texture(uCeiling, vUV).rgb);
        normal = mappedNormal(geometricNormal, vUV, texture(uCeilingNormal, vUV).rgb, 1.0);
        specularStrength = 0.009;
        float age=wearNoise((vPosition.xz+uWorldPhase.xz)*.2+vec2(19,3));
        albedo*=mix(.91,1.0,smoothstep(.18,.65,age));
    } else if (material == 3) {
        albedo = linearColor(vec3(0.34, 0.295, 0.18));
        specularStrength = 0.04;
    } else if (material == 4 || (material >= 32 && material <= 34)) {
        albedo = linearColor(vec3(0.94, 0.92, 0.77));
        float grille = 0.97 + 0.03 * sin(vUV.x * 100.0);
        float level = lampEnvelope(material == 4 ? 0 : material - 32, fract(vMaterial));
        emission = vec3(0.90, 0.86, 0.65) * level * grille;
    } else if (material == 5) {
        // Hidden paint must use normalized [0,1] UVs on its small wall quad.
        if (!uClues || !uFlashlight || flashlight < 0.026) discard;
        vec2 p = vUV - vec2(0.5);
        bool shaft = p.x > -0.36 && p.x < 0.14 && abs(p.y) < 0.052;
        bool tip = p.x >= 0.025 && p.x < 0.36 && abs(p.y) < (0.36 - p.x) * 0.67;
        if (!shaft && !tip) discard;
        albedo = linearColor(vec3(0.53, 0.86, 0.69));
        specularStrength = 0.0;
    } else if (material == 7) {
        albedo = linearColor(vec3(0.43, 0.47, 0.46)) * (0.95 + 0.05 * grain(vUV));
        specularStrength = 0.22;
        shininess = 65.0;
    } else if (material == 13) {
        albedo = linearColor(vec3(0.48, 0.49, 0.46)) * (0.84 + 0.16 * grain(vUV));
        specularStrength = 0.01;
    } else if (material == 16) {
        float letters = exitText(vUV);
        float border = float(vUV.x < 0.025 || vUV.x > 0.975 || vUV.y < 0.045 || vUV.y > 0.955);
        float white = max(letters, border);
        albedo = linearColor(mix(vec3(0.035, 0.42, 0.20), vec3(0.91, 1.0, 0.88), white));
        emission = mix(vec3(0.012, 0.40, 0.095), vec3(1.65, 2.0, 1.60), white);
        specularStrength = 0.02;
    } else if (material == 17) {
        // UVs rotate with the hinged leaf, keeping the oak grain on the door.
        float warp=sin(vUV.y*3.1+sin(vUV.x*9.0))*0.065;
        float rings=sin((vUV.x+warp)*112.0+sin(vUV.y*8.0)*.55);
        float fibers=grain(vec2(vUV.x*3.0,vUV.y*.12));
        vec3 wood=mix(vec3(.25,.13,.055),vec3(.52,.33,.15),.48+.18*rings+.22*fibers);
        albedo=linearColor(wood);
        specularStrength = 0.055;
        shininess = 35.0;
    } else if (material == 25) {
        albedo=linearColor(vec3(.23,.20,.16))*(.86+.14*grain(vUV));
        specularStrength=.025;
    } else if (material == 26) {
        albedo=linearColor(vec3(.42,.26,.23));
        specularStrength=.04;
    } else if (material == 27) {
        albedo=vec3(.92);
        emission=vec3(2.4);
    } else if (material == 19) {
        albedo = linearColor(texture(uHardFloor, vUV).rgb);
        normal = mappedNormal(geometricNormal, vUV, texture(uHardFloorNormal, vUV).rgb, 1.0);
        specularStrength = 0.13;
        shininess = 44.0;
    } else if (material == 24) {
        albedo=linearColor(vec3(.72,.51,.12));
        emission=vec3(.10,.063,.009);
        specularStrength=.65;shininess=70.0;
    } else if (material >= 20 && material <= 22) {
        // All three source materials share sRGB carpet and linear normal
        // images. glTF base factors are already LINEAR and are not decoded.
        // Original mesh UVs preserve its carpet weave across articulated legs.
        vec3 factor = material == 20 ? vec3(0.520995677, 0.502886593, 0.491020888)
                    : material == 21 ? vec3(0.921582043, 0.854992807, 0.806952477)
                                     : vec3(0.00972121768, 0.00856812485, 0.00856812485);
        albedo = linearColor(texture(uCreatureColor, vUV).rgb) * factor;
        normal = mappedNormal(geometricNormal, vUV,
                              texture(uCreatureNormal, vUV).rgb, 0.635);
        specularStrength = 0.012;
        shininess = 12.0;
        // The face receives scene lighting, with a brief fill at capture.
        // No persistent Smiler eye/teeth emission remains on this new model.
        if (material == 21) emission = albedo * (0.30 * uScareGlow);
    }

    // Low ambient fill leaves deep shadows between dim fluorescent pools.
    // Direct lighting uses geometric shadow maps; ambient remains restrained.
    float facingFill = mix(0.83, 1.0, geometricNormal.y * 0.5 + 0.5);
    vec3 ambient = vec3(0.035, 0.034, 0.026) * facingFill;
    vec3 color = albedo * ambient;
    // Match the fixed light capacity; zeroed unused slots contribute no light.
    for (int i = 0; i < 24; ++i) {
        if(uLightPower[i]<=0.0)continue;
        vec3 delta = uLights[i] - vPosition;
        float d = length(delta);
        if (d >= 10.0) continue;
        vec3 lightDirection = delta / max(d, 0.0001);
        float range = max(0.0, 1.0 - d * d / 100.0);
        float power = 0.72 * range * range / (1.0 + 0.14 * d * d);
        // CPU power already contains the same fixture envelope used above.
        power *= max(0.0, uLightPower[i]);
        float diffuse = max(dot(normal, lightDirection), 0.0);
        vec3 halfSum = lightDirection + viewDirection;
        vec3 halfVector = halfSum / max(length(halfSum), 0.0001);
        if(diffuse<=0.0)continue;
        power*=lampVisibility(i,geometricNormal);
        float specular = diffuse > 0.0 ? pow(max(dot(normal, halfVector), 0.0), shininess) : 0.0;
        vec3 lightColor = max(uLightColor[i], vec3(0.0));
        color += (albedo * diffuse + vec3(specular * specularStrength)) * lightColor * power;
    }

    float flashDiffuse = max(dot(normal, flashDirection), 0.0);
    color += albedo * vec3(1.0, 0.98, 0.90) * flashlight * (0.10 + 1.9 * flashDiffuse);
    float flashSpecular = pow(flashDiffuse, shininess) * specularStrength;
    color += vec3(1.0, 0.98, 0.90) * flashlight * flashSpecular * 1.9;
    color *= mix(1.0,0.16,uScareDarken);
    color += emission * (material==21 ? 1.0 : mix(1.0,0.12,uScareDarken));
    if (material == 5) color = albedo * flashlight * 3.0;

    if (uFog) {
        // Exponential fog hides the finite streamed draw radius naturally.
        float fog = 1.0 - exp(-pow(distanceToEye * 0.042, 1.65));
        color = mix(color, vec3(0.003, 0.0032, 0.0018), clamp(fog, 0.0, 0.97));
        // Fade completely before geometry reaches the camera draw limit.
        float drawLimit=max(uRenderDistance, 10.0);
        color *= 1.0-smoothstep(drawLimit*.62,drawLimit-2.0,distanceToEye);
    }
    color *= 1.24 * uExposure;
    color = color / (vec3(1.0) + color);
    color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
    fragColor = vec4(color, 1.0);
}
