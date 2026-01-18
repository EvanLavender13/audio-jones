#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float strength;       // Warp magnitude per iteration
uniform int iterations;       // Cascade depth
uniform int channelMode;      // Channel combination for UV offset
uniform float ridgeAngle;     // Ridge direction (radians)
uniform float anisotropy;     // 0=isotropic, 1=fully directional
uniform float noiseAmount;    // Procedural noise blend (0.0 to 1.0)
uniform float noiseScale;     // Noise frequency

// RGB to HSV for Polar mode
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// Hash and noise functions for procedural terrain
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);  // Smoothstep interpolation

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main()
{
    vec2 warpedUV = fragTexCoord;

    // Precompute ridge direction vectors for anisotropy
    vec2 ridgeDir = vec2(cos(ridgeAngle), sin(ridgeAngle));
    vec2 perpDir = vec2(-ridgeDir.y, ridgeDir.x);

    for (int i = 0; i < iterations; i++) {
        vec3 s = texture(texture0, warpedUV).rgb;
        vec2 offset;

        if (channelMode == 0) {
            // RG
            offset = (s.rg - 0.5) * 2.0;
        } else if (channelMode == 1) {
            // RB
            offset = (s.rb - 0.5) * 2.0;
        } else if (channelMode == 2) {
            // GB
            offset = (s.gb - 0.5) * 2.0;
        } else if (channelMode == 3) {
            // Luminance
            float l = dot(s, vec3(0.299, 0.587, 0.114));
            offset = vec2(l - 0.5) * 2.0;
        } else if (channelMode == 4) {
            // LuminanceSplit
            float l = dot(s, vec3(0.299, 0.587, 0.114));
            offset = vec2(l - 0.5, 0.5 - l) * 2.0;
        } else if (channelMode == 5) {
            // Chrominance
            offset = vec2(s.r - s.b, s.g - s.b) * 2.0;
        } else {
            // Polar (mode 6)
            vec3 hsv = rgb2hsv(s);
            float angle = hsv.x * 6.28318530718;
            offset = vec2(cos(angle), sin(angle)) * hsv.y;
        }

        // Apply directional anisotropy: suppress perpendicular component
        if (anisotropy > 0.0) {
            float ridgeComponent = dot(offset, ridgeDir);
            float perpComponent = dot(offset, perpDir);
            perpComponent *= (1.0 - anisotropy);
            offset = ridgeDir * ridgeComponent + perpDir * perpComponent;
        }

        // Inject procedural noise for terrain features
        if (noiseAmount > 0.0) {
            float n = fbm(warpedUV * noiseScale);
            vec2 noiseOffset = vec2(cos(n * 6.28318), sin(n * 6.28318));
            offset = mix(offset, noiseOffset, noiseAmount);
        }

        warpedUV += offset * strength;
    }

    finalColor = texture(texture0, clamp(warpedUV, 0.0, 1.0));
}
