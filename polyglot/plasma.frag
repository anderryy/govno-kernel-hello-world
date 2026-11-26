#version 330 core
// ============================================================================
// GovnoOS Sacred Plasma + SDF Sphere/Torus + ACES Tonemap + HUD Glyph Shader
// Renders TempleOS plasma interference, smooth-min SDF geometry, Fresnel rim
// reflection, FBM turbulence noise, and bitmap glyph slot uniform checks.
// ============================================================================

in vec2 v_uv;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_resolution;
uniform int u_glyphs[11];

const int EXPECTED_GLYPHS[11] = int[11](
    72, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100
);

float smin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

float sdSphere(vec3 p, float radius) {
    return length(p) - radius;
}

float sdTorus(vec3 p, vec2 t) {
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float fbmNoise(vec2 p) {
    float val = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 4; ++i) {
        vec2 i_p = floor(p);
        vec2 f_p = fract(p);
        f_p = f_p * f_p * (3.0 - 2.0 * f_p);
        float a = hash21(i_p);
        float b = hash21(i_p + vec2(1.0, 0.0));
        float c = hash21(i_p + vec2(0.0, 1.0));
        float d = hash21(i_p + vec2(1.0, 1.0));
        val += amp * mix(mix(a, b, f_p.x), mix(c, d, f_p.x), f_p.y);
        p *= 2.04;
        amp *= 0.5;
    }
    return val;
}

vec3 acesFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 calcNormal(vec3 p) {
    float eps = 0.001;
    return normalize(vec3(
        sdSphere(p + vec3(eps, 0.0, 0.0), 1.0) - sdSphere(p - vec3(eps, 0.0, 0.0), 1.0),
        sdSphere(p + vec3(0.0, eps, 0.0), 1.0) - sdSphere(p - vec3(0.0, eps, 0.0), 1.0),
        sdSphere(p + vec3(0.0, 0.0, eps), 1.0) - sdSphere(p - vec3(0.0, 0.0, eps), 1.0)
    ));
}

float plasmaField(vec2 p, float t) {
    float v1 = sin(p.x * 10.0 + t * 2.1);
    float v2 = sin(10.0 * (p.x * sin(t * 0.5) + p.y * cos(t * 0.33)) + t);
    float v3 = sin(sqrt(dot(p, p) * 100.0 + 1.0) + t * 1.7);
    return (v1 + v2 + v3) * 0.3333;
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * u_resolution.xy) / u_resolution.y;
    vec3 ro = vec3(0.0, 0.0, -3.2);
    vec3 rd = normalize(vec3(uv, 1.4));

    float t = 0.0;
    float hit = -1.0;
    for (int i = 0; i < 48; ++i) {
        vec3 p = ro + rd * t;
        float d1 = sdSphere(p, 1.05 + 0.05 * sin(u_time + p.y * 6.0));
        float d2 = sdTorus(p, vec2(1.35, 0.18));
        float d = smin(d1, d2, 0.25);
        if (d < 0.002) {
            hit = t;
            break;
        }
        t += d;
        if (t > 12.0) break;
    }

    vec3 col = vec3(0.05, 0.02, 0.12);
    if (hit > 0.0) {
        vec3 p = ro + rd * hit;
        vec3 n = calcNormal(p);
        vec3 lightDir = normalize(vec3(sin(u_time), 0.8, -cos(u_time)));
        float diff = max(dot(n, lightDir), 0.12);
        float spec = pow(max(dot(reflect(-lightDir, n), -rd), 0.0), 32.0);
        float fresnel = pow(1.0 - max(dot(n, -rd), 0.0), 3.0);
        col = vec3(0.2, 0.9, 1.0) * diff + vec3(1.0, 0.95, 0.8) * spec + vec3(0.8, 0.2, 1.0) * fresnel;
    } else {
        float pl = plasmaField(uv, u_time);
        float n = fbmNoise(uv * 3.0 + u_time * 0.2);
        col += 0.25 * vec3(0.5 + 0.5 * sin(pl * 3.14), n * 0.4, 0.5 + 0.5 * cos(pl * 3.14));
    }

    int slot = int(clamp(floor(v_uv.x * 11.0), 0.0, 10.0));
    float glyphPulse = float(u_glyphs[slot] ^ EXPECTED_GLYPHS[slot]);
    col.g += (glyphPulse == 0.0) ? 0.08 : 0.0;

    FragColor = vec4(acesFilm(col), 1.0);
}
