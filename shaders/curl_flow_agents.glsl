#version 430

// Curl noise flow: agents follow divergence-free curl noise field, deposit colored trails
// Density modulates the potential field (Bridson 2007), making flow tangential to bright areas

layout(local_size_x = 1024) in;

struct Agent {
    float x;
    float y;
    float velocityAngle;
    float _pad1;
    float _pad2;
    float _pad3;
    float _pad4;
    float _pad5;
};

layout(std430, binding = 0) buffer AgentBuffer {
    Agent agents[];
};

layout(rgba32f, binding = 1) uniform image2D trailMap;
layout(binding = 3) uniform sampler2D colorLUT;
layout(binding = 4) uniform sampler3D noiseTexture;
layout(binding = 5) uniform sampler2D gradientMap;

uniform vec2 resolution;
uniform float time;
uniform float noiseFrequency;
uniform float noiseEvolution;
uniform float trailInfluence;    // Density influence on flow field (0 = pure noise, 1 = full obstacle)
uniform float stepSize;
uniform float depositAmount;
uniform float value;

const float PI = 3.14159265359;

// Standard luminance weights
const vec3 LUMA_WEIGHTS = vec3(0.299, 0.587, 0.114);

// Sample curl vector from precomputed 3D noise texture.
// Texture stores curl XY in RG channels.
vec2 sampleNoiseCurl(vec3 noisePos)
{
    return texture(noiseTexture, noisePos).rg;
}

// Wrap position to valid range (handles negative values correctly)
vec2 wrapPosition(vec2 pos)
{
    return mod(mod(pos, resolution) + resolution, resolution);
}

// Sample trail density at position
float sampleTrailDensity(vec2 pos)
{
    ivec2 coord = ivec2(wrapPosition(pos));
    vec3 color = imageLoad(trailMap, coord).rgb;
    return dot(color, LUMA_WEIGHTS);
}

// Compute density-modulated curl (Bridson 2007)
// Uses precomputed noise texture and gradient map for efficiency.
// Flow bends around bright areas via density ramp modulation.
vec2 computeModulatedCurl(vec3 noisePos, vec2 screenPos, float influence)
{
    // Sample curl from precomputed 3D noise texture
    vec2 noiseCurl = sampleNoiseCurl(noisePos);

    if (influence < 0.001) {
        return noiseCurl;
    }

    // Sample precomputed density and gradient from gradient map
    vec2 uv = screenPos / resolution;
    vec2 gradientSample = texture(gradientMap, uv).rg;
    float d = sampleTrailDensity(screenPos);

    // Ramp function: 1 near zero density, 0 near full density
    float ramp = 1.0 - clamp(d * influence, 0.0, 1.0);
    // Gradient of ramp = -influence * gradient of density
    vec2 rampGrad = -influence * gradientSample;

    // curl(ramp) = (dramp/dy, -dramp/dx)
    vec2 rampCurl = vec2(rampGrad.y, -rampGrad.x);

    // Modulated curl: ramp * noise_curl + ramp_curl (potential term approximated)
    return ramp * noiseCurl + rampCurl;
}

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= agents.length()) {
        return;
    }

    Agent agent = agents[id];
    vec2 pos = vec2(agent.x, agent.y);

    // Compute density-modulated curl noise velocity
    // Flow field bends around bright areas (Bridson 2007)
    vec3 noisePos = vec3(pos * noiseFrequency, time * noiseEvolution);
    vec2 curl = computeModulatedCurl(noisePos, pos, trailInfluence);

    // Normalize curl for consistent movement direction
    // If curl is near-zero, use previous velocity direction to keep moving
    float curlLen = length(curl);
    if (curlLen > 0.001) {
        curl = curl / curlLen;
    } else {
        // Use stored velocity angle as fallback direction
        curl = vec2(cos(agent.velocityAngle), sin(agent.velocityAngle));
    }

    // Update position and wrap at boundaries
    pos = wrapPosition(pos + curl * stepSize);

    // Store velocity angle for color mapping
    float velocityAngle = atan(curl.y, curl.x);

    agent.x = pos.x;
    agent.y = pos.y;
    agent.velocityAngle = velocityAngle;

    float t = (velocityAngle + PI) / (2.0 * PI);
    vec3 lutColor = texture(colorLUT, vec2(t, 0.5)).rgb;
    vec3 depositColor = lutColor * value;

    // Deposit trail at new position
    ivec2 newCoord = ivec2(pos);
    vec4 current = imageLoad(trailMap, newCoord);
    vec3 newColor = current.rgb + depositColor * depositAmount;

    // Proportional scaling to prevent overflow
    float maxChan = max(newColor.r, max(newColor.g, newColor.b));
    if (maxChan > 1.0) {
        newColor /= maxChan;
    }

    imageStore(trailMap, newCoord, vec4(newColor, 0.0));

    agents[id] = agent;
}
