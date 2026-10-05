#version 330 core

in vec3 vPosition;
in vec3 vNormal;
in vec2 vUV;
flat in float vMaterial;

out vec4 fragColor;

uniform vec3 uCamera;
uniform vec3 uForward;
uniform vec3 uWorldPhase;
uniform float uTime;
uniform float uExposure;
uniform bool uFlashlight;
uniform bool uClues;
uniform bool uFog;
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

// Texture and authored material RGB values are display-space colors. All
// illumination is linear, with a single tone map and gamma conversion below.
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

void main() {
    // Integer material IDs remain unchanged; fluorescent tubes additionally
    // encode their stable phase in the fractional part of IDs 32, 33 and 34.
    int material = int(floor(vMaterial));
    vec3 geometricNormal = normalize(vNormal);
    vec3 normal = geometricNormal;
    vec3 toEye = uCamera - vPosition;
    float distanceToEye = length(toEye);
    vec3 viewDirection = toEye / max(distanceToEye, 0.0001);
    float cone = smoothstep(0.84, 0.955, dot(-viewDirection, normalize(uForward)));
    float flashlight = uFlashlight ? cone / (1.0 + distanceToEye * distanceToEye * 0.115) : 0.0;

    vec3 albedo = linearColor(vec3(0.50));
    vec3 emission = vec3(0.0);
    float specularStrength = 0.035;
    float shininess = 20.0;
    if (material == 0) {
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
    } else if (material == 2) {
        albedo = linearColor(texture(uCeiling, vUV).rgb);
        normal = mappedNormal(geometricNormal, vUV, texture(uCeilingNormal, vUV).rgb, 1.0);
        specularStrength = 0.009;
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
        albedo = linearColor(vec3(0.14, 0.39, 0.34));
        specularStrength = 0.09;
        shininess = 35.0;
    } else if (material == 19) {
        albedo = linearColor(texture(uHardFloor, vUV).rgb);
        normal = mappedNormal(geometricNormal, vUV, texture(uHardFloorNormal, vUV).rgb, 1.0);
        specularStrength = 0.13;
        shininess = 44.0;
    }

    // Low ambient fill leaves deep shadows between dim fluorescent pools.
    // This is direct Blinn-Phong lighting, without shadow maps or GI.
    float facingFill = mix(0.83, 1.0, geometricNormal.y * 0.5 + 0.5);
    vec3 ambient = vec3(0.035, 0.034, 0.026) * facingFill;
    vec3 color = albedo * ambient;
    // Match the fixed light capacity; zeroed unused slots contribute no light.
    for (int i = 0; i < 24; ++i) {
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
        float specular = diffuse > 0.0 ? pow(max(dot(normal, halfVector), 0.0), shininess) : 0.0;
        vec3 lightColor = max(uLightColor[i], vec3(0.0));
        color += (albedo * diffuse + vec3(specular * specularStrength)) * lightColor * power;
    }

    float flashDiffuse = max(dot(normal, viewDirection), 0.0);
    color += albedo * vec3(1.0, 0.98, 0.90) * flashlight * (0.10 + 1.9 * flashDiffuse);
    float flashSpecular = pow(flashDiffuse, shininess) * specularStrength;
    color += vec3(1.0, 0.98, 0.90) * flashlight * flashSpecular * 1.9;
    color += emission;
    if (material == 5) color = albedo * flashlight * 3.0;

    if (uFog) {
        // Exponential fog hides the finite streamed draw radius naturally.
        float fog = 1.0 - exp(-pow(distanceToEye * 0.042, 1.65));
        color = mix(color, vec3(0.003, 0.0032, 0.0018), clamp(fog, 0.0, 0.97));
    }
    color *= 1.24 * uExposure;
    color = color / (vec3(1.0) + color);
    color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
    fragColor = vec4(color, 1.0);
}
