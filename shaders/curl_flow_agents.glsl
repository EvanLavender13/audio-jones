#version 430

// Curl noise flow: agents follow divergence-free curl noise field, deposit colored trails
// Exactly like physarum but uses curl noise instead of sensor-based steering

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
layout(binding = 2) uniform sampler2D accumMap;

uniform vec2 resolution;
uniform float time;
uniform float noiseFrequency;
uniform float noiseEvolution;
uniform float trailInfluence;
uniform float accumSenseBlend;  // 0 = trail only, 1 = accum only
uniform float stepSize;
uniform float depositAmount;
uniform float saturation;
uniform float value;

const float PI = 3.14159265359;

// Standard luminance weights
const vec3 LUMA_WEIGHTS = vec3(0.299, 0.587, 0.114);

// HSV to RGB conversion
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// 3D Simplex noise with analytical gradient
vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x) { return mod289(((x * 34.0) + 1.0) * x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

vec4 snoise3_grad(vec3 v)
{
    const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

    vec3 i = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);

    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;

    i = mod289(i);
    vec4 p = permute(permute(permute(
              i.z + vec4(0.0, i1.z, i2.z, 1.0))
            + i.y + vec4(0.0, i1.y, i2.y, 1.0))
            + i.x + vec4(0.0, i1.x, i2.x, 1.0));

    float n_ = 0.142857142857;
    vec3 ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);

    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);

    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);

    vec4 norm = taylorInvSqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    vec4 m = max(0.6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    vec4 m2 = m * m;
    vec4 m4 = m2 * m2;

    vec4 pdotx = vec4(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3));

    vec4 temp = m2 * m * pdotx;
    vec3 gradient = -8.0 * (temp.x * x0 + temp.y * x1 + temp.z * x2 + temp.w * x3);
    gradient += m4.x * p0 + m4.y * p1 + m4.z * p2 + m4.w * p3;
    gradient *= 42.0;

    float noiseValue = 42.0 * dot(m4, pdotx);

    return vec4(noiseValue, gradient);
}

// Compute 2D curl from 3D noise gradient
// For scalar field N(x,y,z), 2D curl at fixed z is: (dN/dy, -dN/dx)
vec2 computeCurl(vec3 pos)
{
    vec4 n = snoise3_grad(pos);
    // n.x = noise value, n.yzw = gradient (dN/dx, dN/dy, dN/dz)
    return vec2(n.z, -n.y);
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

// Sample accum (feedback) density at position
float sampleAccumDensity(vec2 pos)
{
    vec2 uv = pos / resolution;
    vec3 color = texture(accumMap, uv).rgb;
    return dot(color, LUMA_WEIGHTS);
}

// Sample blended density (trail vs accum based on accumSenseBlend)
float sampleBlendedDensity(vec2 pos)
{
    float trail = sampleTrailDensity(pos);
    float accum = sampleAccumDensity(pos);
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

    // Sample blended density at current position (trail vs feedback)
    float density = sampleBlendedDensity(pos);

    // Compute curl noise velocity
    vec3 noisePos = vec3(pos * noiseFrequency, time * noiseEvolution);
    vec2 curl = computeCurl(noisePos);

    // Normalize curl for consistent movement direction
    // If curl is near-zero, use previous velocity direction to keep moving
    float curlLen = length(curl);
    if (curlLen > 0.001) {
        curl = curl / curlLen;
    } else {
        // Use stored velocity angle as fallback direction
        curl = vec2(cos(agent.velocityAngle), sin(agent.velocityAngle));
    }

    // Speed modulated by density (slower in dense areas, but always move)
    float speedMod = 1.0 - trailInfluence * clamp(density, 0.0, 1.0);
    float speed = stepSize * max(speedMod, 0.1);  // Minimum 10% speed

    // Update position and wrap at boundaries
    pos = wrapPosition(pos + curl * speed);

    // Store velocity angle for color mapping
    float velocityAngle = atan(curl.y, curl.x);

    agent.x = pos.x;
    agent.y = pos.y;
    agent.velocityAngle = velocityAngle;

    // Compute deposit color from velocity angle
    float hue = (velocityAngle + PI) / (2.0 * PI);
    vec3 depositColor = hsv2rgb(vec3(hue, saturation, value));

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
