#version 330

// Chladni Warp: UV displacement using Chladni plate equations
// Gradient of Chladni function displaces toward/along nodal lines

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float n;
uniform float m;
uniform float plateSize;
uniform float strength;
uniform int warpMode;
uniform float animPhase;
uniform float animRange;
uniform bool preFold;

out vec4 finalColor;

const float PI = 3.14159265;

float chladni(vec2 uv, float n_val, float m_val, float L)
{
    float nx = n_val * PI / L;
    float mx = m_val * PI / L;
    return cos(nx * uv.x) * cos(mx * uv.y) - cos(mx * uv.x) * cos(nx * uv.y);
}

vec2 chladniGradient(vec2 uv, float n_val, float m_val, float L)
{
    float nx = n_val * PI / L;
    float mx = m_val * PI / L;

    float dfdx = -nx * sin(nx * uv.x) * cos(mx * uv.y)
                 + mx * sin(mx * uv.x) * cos(nx * uv.y);
    float dfdy = -mx * cos(nx * uv.x) * sin(mx * uv.y)
                 + nx * cos(mx * uv.x) * sin(nx * uv.y);

    return vec2(dfdx, dfdy);
}

vec2 chladniWarp(vec2 uv, float n_val, float m_val, float L, float str, int mode)
{
    float f = chladni(uv, n_val, m_val, L);
    vec2 grad = chladniGradient(uv, n_val, m_val, L);
    float gradLen = length(grad);

    if (gradLen < 0.001) {
        return uv;
    }

    vec2 dir = grad / gradLen;

    if (mode == 0) {
        // Toward nodes: sand accumulation effect
        return uv - dir * str * f;
    } else if (mode == 1) {
        // Along nodes: stream effect perpendicular to gradient
        vec2 perp = vec2(-dir.y, dir.x);
        return uv + perp * str * f;
    } else {
        // Intensity-based: stronger near antinodes
        return uv + dir * str * abs(f);
    }
}

void main()
{
    // Map UV from [0,1] to [-1,1] centered coordinates
    vec2 centered = fragTexCoord * 2.0 - 1.0;

    // Optional symmetry fold before warp
    if (preFold) {
        centered = abs(centered);
    }

    // Animate n and m parameters
    float n_anim = n + animRange * sin(animPhase);
    float m_anim = m + animRange * cos(animPhase);

    // Apply Chladni warp
    vec2 warped = chladniWarp(centered, n_anim, m_anim, plateSize, strength, warpMode);

    // Map back to [0,1] UV space
    vec2 uv = (warped + 1.0) * 0.5;

    finalColor = texture(texture0, uv);
}
