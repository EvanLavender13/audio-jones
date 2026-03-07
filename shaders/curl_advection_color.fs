#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;        // visual ping-pong read (auto-bound by DrawTextureRec)
uniform sampler2D stateTexture;    // fresh state from pass 1
uniform sampler2D colorLUT;        // velocity angle -> color
uniform vec2 resolution;
uniform float decayFactor;         // exp(-0.693147 * dt / halfLife)
uniform float value;               // brightness from color config
uniform int diffusionScale;        // tap spacing (0 = no blur)

const float PI = 3.14159265359;

// 5-tap Gaussian weights matching TrailMap's trail_diffusion.glsl
const float W0 = 0.0625;
const float W1 = 0.25;
const float W2 = 0.375;

// Sample texture0 with cross-kernel diffusion blur
vec3 samplePrevious(vec2 uv, vec2 texel, float ds) {
    // Horizontal 5-tap
    vec3 h = texture(texture0, uv + vec2(-2.0*ds*texel.x, 0)).rgb * W0
           + texture(texture0, uv + vec2(-ds*texel.x, 0)).rgb * W1
           + texture(texture0, uv).rgb * W2
           + texture(texture0, uv + vec2(ds*texel.x, 0)).rgb * W1
           + texture(texture0, uv + vec2(2.0*ds*texel.x, 0)).rgb * W0;

    // Vertical 5-tap
    vec3 v = texture(texture0, uv + vec2(0, -2.0*ds*texel.y)).rgb * W0
           + texture(texture0, uv + vec2(0, -ds*texel.y)).rgb * W1
           + texture(texture0, uv).rgb * W2
           + texture(texture0, uv + vec2(0, ds*texel.y)).rgb * W1
           + texture(texture0, uv + vec2(0, 2.0*ds*texel.y)).rgb * W0;

    return (h + v) * 0.5;
}

void main() {
    vec2 uv = fragTexCoord;
    vec3 state = texture(stateTexture, uv).xyz;

    // Color: velocity angle -> LUT
    float angle = atan(state.y, state.x);
    float t = (angle + PI) / (2.0 * PI);
    vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;
    float brightness = length(state.xy);

    vec3 newColor = color * brightness * value;

    // Previous frame with optional diffusion
    vec3 previous;
    if (diffusionScale > 0) {
        vec2 texel = 1.0 / resolution;
        previous = samplePrevious(uv, texel, float(diffusionScale));
    } else {
        previous = texture(texture0, uv).rgb;
    }

    vec3 result = max(newColor, previous * decayFactor);

    finalColor = vec4(result, 1.0);
}
