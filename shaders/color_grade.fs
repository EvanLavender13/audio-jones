#version 330

// Color Grade: Full-spectrum color manipulation
// Applies brightness, temperature, contrast, saturation, lift/gamma/gain, and hue shift

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float hueShift;         // 0-1 (normalized from 0-360 degrees)
uniform float saturation;       // 0-2, default 1
uniform float brightness;       // -2 to +2 F-stops, default 0
uniform float contrast;         // 0.5-2, default 1
uniform float temperature;      // -1 to +1, default 0
uniform float shadowsOffset;    // -0.5 to +0.5, default 0
uniform float midtonesOffset;   // -0.5 to +0.5, default 0
uniform float highlightsOffset; // -0.5 to +0.5, default 0

out vec4 finalColor;

// RGB to HSV (GPU Gems optimized)
vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Hue shift in HSV space
vec3 applyHueShift(vec3 color, float shift)
{
    vec3 hsv = rgb2hsv(color);
    hsv.x = fract(hsv.x + shift);
    return hsv2rgb(hsv);
}

// Saturation via luminance lerp (Filmic Worlds weights)
vec3 applySaturation(vec3 color, float sat)
{
    vec3 lumaWeights = vec3(0.25, 0.50, 0.25);
    float grey = dot(lumaWeights, color);
    return vec3(grey) + sat * (color - vec3(grey));
}

// Exposure in F-stops
vec3 applyBrightness(vec3 color, float exposure)
{
    return color * exp2(exposure);
}

// Log-space contrast around 18% grey (Filmic Worlds)
float logContrast(float x, float con)
{
    float eps = 1e-6;
    float logMidpoint = log2(0.18);
    float logX = log2(x + eps);
    float adjX = logMidpoint + (logX - logMidpoint) * con;
    return max(0.0, exp2(adjX) - eps);
}

vec3 applyContrast(vec3 color, float con)
{
    return vec3(
        logContrast(color.r, con),
        logContrast(color.g, con),
        logContrast(color.b, con)
    );
}

// Temperature: warm/cool RGB scaling
vec3 applyTemperature(vec3 color, float temp)
{
    float warmth = temp * 0.3;
    return color * vec3(1.0 + warmth, 1.0, 1.0 - warmth);
}

// Lift/Gamma/Gain per channel (Filmic Worlds)
float liftGammaGain(float x, float lift, float gamma, float gain)
{
    // Clamp negative values: pow(negative, fractional) is undefined in GLSL
    float lerpV = pow(max(0.0, x), gamma);
    return gain * lerpV + lift * (1.0 - lerpV);
}

vec3 applyShadowsMidtonesHighlights(vec3 color, float shadows, float midtones, float highlights)
{
    float lift = shadows;
    float gain = 1.0 + highlights;
    float gamma = 1.0 / max(0.01, 1.0 + midtones);
    return vec3(
        liftGammaGain(color.r, lift, gamma, gain),
        liftGammaGain(color.g, lift, gamma, gain),
        liftGammaGain(color.b, lift, gamma, gain)
    );
}

void main()
{
    vec3 color = texture(texture0, fragTexCoord).rgb;

    // 1. Brightness/Exposure (linear operation first)
    color = applyBrightness(color, brightness);

    // 2. Temperature shift
    color = applyTemperature(color, temperature);

    // 3. Contrast (log space preserves shadows)
    color = applyContrast(color, contrast);

    // 4. Saturation
    color = applySaturation(color, saturation);

    // 5. Lift/Gamma/Gain
    color = applyShadowsMidtonesHighlights(color, shadowsOffset, midtonesOffset, highlightsOffset);

    // 6. Hue shift (last - operates on final color relationships)
    color = applyHueShift(color, hueShift);

    finalColor = vec4(color, 1.0);
}
