#version 430

// Physarum algorithm with hue-based species sensing and color deposition

layout(local_size_x = 1024) in;

struct Agent {
    float x;
    float y;
    float heading;
    float hue;         // Agent's hue identity (0-1) for deposit color and affinity
    float spectrumPos; // Position in color distribution (0-1) for FFT lookup
    float _pad1;
    float _pad2;
    float _pad3;
};

layout(std430, binding = 0) buffer AgentBuffer {
    Agent agents[];
};

layout(rgba32f, binding = 1) uniform image2D trailMap;
layout(binding = 2) uniform sampler2D accumMap;
layout(binding = 3) uniform sampler2D fftMap;

uniform vec2 resolution;
uniform float frequencyModulation;
uniform float beatIntensity;
uniform float stepBeatModulation;
uniform float sensorBeatModulation;
uniform float accumSenseBlend;  // 0 = trail only, 1 = accum only
uniform float sensorDistance;
uniform float sensorAngle;
uniform float turningAngle;
uniform float stepSize;
uniform float depositAmount;
uniform float time;
uniform float saturation;
uniform float value;

// Standard luminance weights (Rec. 601)
const vec3 LUMA_WEIGHTS = vec3(0.299, 0.587, 0.114);

// Hash for stochastic behavior (based on Sage Jenson's approach)
uint hash(uint state)
{
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    return state;
}

// RGB to HSV conversion
vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// HSV to RGB conversion
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Compute hue difference with wraparound handling (hue is circular 0-1)
float hueDifference(float h1, float h2)
{
    float diff = abs(h1 - h2);
    return min(diff, 1.0 - diff);
}

// Sample FFT energy at spectrum position (0-1 maps to 20Hz-20kHz on log scale)
// Returns normalized magnitude 0-1
float sampleFFT(float spectrumPos)
{
    // Log scale mapping: spectrumPos 0→20Hz, 1→20kHz
    // log10(20) ≈ 1.301, log10(20000) ≈ 4.301, range = 3.0
    const float LOG_MIN_FREQ = 1.30103;  // log10(20)
    const float LOG_RANGE = 3.0;         // log10(20000) - log10(20)
    const float NYQUIST = 24000.0;       // 48kHz sample rate / 2
    const float NUM_BINS = 1025.0;

    float logFreq = LOG_MIN_FREQ + spectrumPos * LOG_RANGE;
    float freq = pow(10.0, logFreq);

    // Convert frequency to normalized bin position (0-1)
    float binPos = (freq / NYQUIST) * (NUM_BINS - 1.0) / NUM_BINS;
    binPos = clamp(binPos, 0.0, 1.0);

    // Sample 1D texture (stored as 1025x1)
    return texture(fftMap, vec2(binPos, 0.5)).r;
}

// Compute affinity from color (lower = more attractive)
// Blends hue similarity with intensity: agents prefer dense areas of similar hue
// In solid color mode, hueDiff ≈ 0 so intensity dominates
// In rainbow mode, hue provides primary gradient, intensity secondary
// FFT modulation amplifies hue difference when agent's frequency band is loud
float computeAffinity(vec3 color, float agentHue, float agentSpectrumPos)
{
    float intensity = dot(color, LUMA_WEIGHTS);

    if (intensity < 0.001) {
        return 1.0;  // No content = least attractive
    }

    vec3 hsv = rgb2hsv(color);
    float hueDiff = hueDifference(agentHue, hsv.x);

    // FFT modulation: louder frequency → stronger repulsion from different hues
    // sqrt curve boosts mid-range values for more responsive feel
    float fftEnergy = sqrt(sampleFFT(agentSpectrumPos));
    float modulation = 1.0 + fftEnergy * frequencyModulation * 3.0;
    float modulatedHueDiff = hueDiff * modulation;

    return modulatedHueDiff + (1.0 - intensity) * 0.3;
}

float sampleTrailAffinity(vec2 pos, float agentHue, float agentSpectrumPos)
{
    ivec2 coord = ivec2(mod(pos, resolution));
    vec3 trailColor = imageLoad(trailMap, coord).rgb;
    return computeAffinity(trailColor, agentHue, agentSpectrumPos);
}

float sampleAccumAffinity(vec2 pos, float agentHue, float agentSpectrumPos)
{
    vec2 uv = pos / resolution;
    vec3 accumColor = texture(accumMap, uv).rgb;
    return computeAffinity(accumColor, agentHue, agentSpectrumPos);
}

// Sample and blend trail + accum affinity at position
float sampleBlendedAffinity(vec2 pos, float agentHue, float agentSpectrumPos)
{
    float trail = sampleTrailAffinity(pos, agentHue, agentSpectrumPos);
    float accum = sampleAccumAffinity(pos, agentHue, agentSpectrumPos);
    return mix(trail, accum, accumSenseBlend);
}

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= agents.length()) {
        return;
    }

    Agent agent = agents[id];
    vec2 pos = vec2(agent.x, agent.y);

    // Beat-modulated sensor distance: see further on beats
    float modulatedSensor = sensorDistance * (1.0 + beatIntensity * sensorBeatModulation);

    // Sensor positions (Jones 2010: three forward-facing sensors)
    vec2 frontDir = vec2(cos(agent.heading), sin(agent.heading));
    vec2 leftDir = vec2(cos(agent.heading + sensorAngle), sin(agent.heading + sensorAngle));
    vec2 rightDir = vec2(cos(agent.heading - sensorAngle), sin(agent.heading - sensorAngle));

    vec2 frontPos = pos + frontDir * modulatedSensor;
    vec2 leftPos = pos + leftDir * modulatedSensor;
    vec2 rightPos = pos + rightDir * modulatedSensor;

    // Sample blended affinity (lower = more attractive)
    float front = sampleBlendedAffinity(frontPos, agent.hue, agent.spectrumPos);
    float left = sampleBlendedAffinity(leftPos, agent.hue, agent.spectrumPos);
    float right = sampleBlendedAffinity(rightPos, agent.hue, agent.spectrumPos);

    uint hashState = hash(id + uint(time * 1000.0));
    float rnd = float(hashState) / 4294967295.0;

    // Turn toward LOWEST hue difference (most similar color)
    if (front < left && front < right) {
        // Front is most similar, no turn needed
    } else if (front > left && front > right) {
        // Both sides more similar: random turn
        agent.heading += (rnd - 0.5) * turningAngle * 2.0;
    } else if (left < right) {
        // Left is more similar
        agent.heading += turningAngle;
    } else if (right < left) {
        // Right is more similar
        agent.heading -= turningAngle;
    }

    // Beat-modulated step size: faster on beats
    float modulatedStep = stepSize * (1.0 + beatIntensity * stepBeatModulation);

    // Move forward in the NEW heading direction (after turning)
    vec2 moveDir = vec2(cos(agent.heading), sin(agent.heading));
    pos += moveDir * modulatedStep;

    // Wrap at boundaries
    pos = mod(pos, resolution);

    agent.x = pos.x;
    agent.y = pos.y;

    // Deposit color based on agent hue
    ivec2 coord = ivec2(pos);
    vec3 depositColor = hsv2rgb(vec3(agent.hue, saturation, value));
    vec4 current = imageLoad(trailMap, coord);
    vec3 newColor = current.rgb + depositColor * depositAmount;

    // Scale proportionally to prevent overflow while preserving color ratios (saturation)
    // Independent clamping causes desaturation: (1.2, 0.3, 0.1) → (1.0, 0.3, 0.1)
    // Proportional scaling preserves hue: (1.2, 0.3, 0.1) → (1.0, 0.25, 0.083)
    float maxChan = max(newColor.r, max(newColor.g, newColor.b));
    if (maxChan > 1.0) {
        newColor /= maxChan;
    }

    imageStore(trailMap, coord, vec4(newColor, 0.0));

    agents[id] = agent;
}
