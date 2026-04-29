// Based on "rps cell nomming :3" by frisk256
// https://www.shadertoy.com/view/NcjXRh
// License: CC BY-NC-SA 3.0 Unported
// Modified: 3 species packed into 2 RGBA textures, single shader with passIndex,
//           parameterized via uniforms (feed/kill/coupling/warp/diffusion/reaction)
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;       // statePingPong0[readIdx0] (species 0 rg, species 1 ba)
uniform sampler2D stateTex1;      // statePingPong1[readIdx1] (species 2 rg)
uniform vec2 resolution;
uniform float time;
uniform int passIndex;            // 0 = output species 0+1; 1 = output species 2

uniform float feedRate;
uniform float killRateBase;
uniform float couplingStrength;
uniform float predatorAdvantage;
uniform float warpIntensity;
uniform float warpSpeed;
uniform float activatorRadius;
uniform float inhibitorRadius;
uniform int reactionSteps;
uniform float reactionRate;

const float PI = 3.14159265358979;

void main() {
    vec2 r = resolution;
    vec2 p = fragTexCoord * r;

    vec4 hp0 = texture(texture0, p / r);
    vec4 hp1 = texture(stateTex1, p / r);
    float c0Center = hp0.x;
    float c1Center = hp0.z;
    float c2Center = hp1.x;

    float q = 1.0;
    for (int i = 0; i < 8; i++) {
        p.x += sin(p.y / r.y * PI * 2.0 * q + time * (fract(q / 4.0) - 0.3) * warpSpeed) / q * warpIntensity;
        p.y -= sin(p.x / r.x * PI * 2.0 * q + time * (fract(q / 5.0) - 0.2) * warpSpeed) / q * warpIntensity;
        q *= -1.681;
        q = fract(q / 10.0) * 10.0 - 5.0;
        q = floor(q) + 1.0;
    }

    vec4 self0 = texture(texture0, p / r);
    vec4 self1 = texture(stateTex1, p / r);

    // Asymmetric diffusion - activator at activatorRadius, inhibitor at inhibitorRadius.
    // For each species, sum self + 4 cardinal neighbors then divide by 5.
    vec2 c0 = self0.xy;
    c0.x += texture(texture0, (p + vec2(activatorRadius, 0.0)) / r).x;
    c0.x += texture(texture0, (p + vec2(0.0, activatorRadius)) / r).x;
    c0.x += texture(texture0, (p - vec2(activatorRadius, 0.0)) / r).x;
    c0.x += texture(texture0, (p - vec2(0.0, activatorRadius)) / r).x;
    c0.y += texture(texture0, (p + vec2(inhibitorRadius, 0.0)) / r).y;
    c0.y += texture(texture0, (p + vec2(0.0, inhibitorRadius)) / r).y;
    c0.y += texture(texture0, (p - vec2(inhibitorRadius, 0.0)) / r).y;
    c0.y += texture(texture0, (p - vec2(0.0, inhibitorRadius)) / r).y;
    c0 /= 5.0;

    vec2 c1 = self0.zw;
    c1.x += texture(texture0, (p + vec2(activatorRadius, 0.0)) / r).z;
    c1.x += texture(texture0, (p + vec2(0.0, activatorRadius)) / r).z;
    c1.x += texture(texture0, (p - vec2(activatorRadius, 0.0)) / r).z;
    c1.x += texture(texture0, (p - vec2(0.0, activatorRadius)) / r).z;
    c1.y += texture(texture0, (p + vec2(inhibitorRadius, 0.0)) / r).w;
    c1.y += texture(texture0, (p + vec2(0.0, inhibitorRadius)) / r).w;
    c1.y += texture(texture0, (p - vec2(inhibitorRadius, 0.0)) / r).w;
    c1.y += texture(texture0, (p - vec2(0.0, inhibitorRadius)) / r).w;
    c1 /= 5.0;

    vec2 c2 = self1.xy;
    c2.x += texture(stateTex1, (p + vec2(activatorRadius, 0.0)) / r).x;
    c2.x += texture(stateTex1, (p + vec2(0.0, activatorRadius)) / r).x;
    c2.x += texture(stateTex1, (p - vec2(activatorRadius, 0.0)) / r).x;
    c2.x += texture(stateTex1, (p - vec2(0.0, activatorRadius)) / r).x;
    c2.y += texture(stateTex1, (p + vec2(inhibitorRadius, 0.0)) / r).y;
    c2.y += texture(stateTex1, (p + vec2(0.0, inhibitorRadius)) / r).y;
    c2.y += texture(stateTex1, (p - vec2(inhibitorRadius, 0.0)) / r).y;
    c2.y += texture(stateTex1, (p - vec2(0.0, inhibitorRadius)) / r).y;
    c2 /= 5.0;

    // Cyclic coupling on the activator (.x) channel - matches reference's GLSL truncation
    // of vec2(scalar, vec4) which takes .x of the vec4 expression.
    // species 0: predator = c2, prey = c1
    // species 1: predator = c0, prey = c2
    // species 2: predator = c1, prey = c0
    // K is frozen across the reaction loop (just like hp/hn are sampled outside the loop in reference).
    float k0 = couplingStrength * (c2Center - c1Center * predatorAdvantage) - killRateBase;
    float k1 = couplingStrength * (c0Center - c2Center * predatorAdvantage) - killRateBase;
    float k2 = couplingStrength * (c1Center - c0Center * predatorAdvantage) - killRateBase;

    // Gray-Scott reaction: du = -u*v^2 + F*(1-u); dv = +u*v^2 + K*v
    for (int n = 0; n < reactionSteps; n++) {
        c0 += (vec2(-1.0, 1.0) * c0.x * c0.y * c0.y + vec2(feedRate, k0) * vec2(1.0 - c0.x, c0.y)) * reactionRate;
        c1 += (vec2(-1.0, 1.0) * c1.x * c1.y * c1.y + vec2(feedRate, k1) * vec2(1.0 - c1.x, c1.y)) * reactionRate;
        c2 += (vec2(-1.0, 1.0) * c2.x * c2.y * c2.y + vec2(feedRate, k2) * vec2(1.0 - c2.x, c2.y)) * reactionRate;
    }

    c0 = clamp(c0, 0.0, 1.0);
    c1 = clamp(c1, 0.0, 1.0);
    c2 = clamp(c2, 0.0, 1.0);

    if (passIndex == 0) {
        finalColor = vec4(c0.x, c0.y, c1.x, c1.y);
    } else {
        finalColor = vec4(c2.x, c2.y, 0.0, 0.0);
    }
}
