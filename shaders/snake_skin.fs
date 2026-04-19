// Based on "Rainbow Snake" by Martijn Steinrucken aka BigWings (2015)
// https://www.shadertoy.com/view/4dc3R2
// License: CC BY-NC-SA 3.0 Unported
// Modified: gradient LUT coloring, per-scale FFT reactivity, uniform shape
// (no spatial morph), CPU-accumulated phases, aspect ratio correction

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float scaleSize;
uniform float spikeAmount;
uniform float spikeFrequency;
uniform float sag;
uniform float scrollPhase;
uniform float wavePhase;
uniform float waveAmplitude;
uniform float twinklePhase;
uniform float twinkleIntensity;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform float sampleRate;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;

#define MOD3 vec3(.1031, .11369, .13787)

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float brightness(vec2 uv, vec2 id) {
    float n = hash12(id);
    float c = mix(0.7, 1.0, n);

    float x = abs(id.x - scaleSize * 0.5);
    float stripes = sin(x * 0.65 + sin(id.y * 0.5) + 0.3) * 0.5 + 0.5;
    stripes = pow(stripes, 4.0);
    c *= 1.0 - stripes * 0.5;

    float twinkle = sin(twinklePhase + n * 6.28) * 0.5 + 0.5;
    twinkle = pow(twinkle, 40.0);
    c += twinkle * twinkleIntensity;

    float freq = baseFreq * pow(maxFreq / baseFreq, n);
    float bin = freq / (sampleRate * 0.5);
    float energy = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
    float audioBright = baseBright + mag;

    return c * audioBright;
}

float spokes(vec2 uv, float spikeFreq) {
    vec2 p = vec2(0.0, 1.0);
    vec2 d = p - uv;
    float l = length(d);
    d /= l;

    vec2 up = vec2(1.0, 0.0);

    float c = dot(up, d);
    c = abs(sin(c * spikeFreq));
    c *= l;

    return c;
}

vec4 ScaleInfo(vec2 uv, vec2 p, vec3 shape) {
    float spikeAmt = shape.x;
    float spikeFreq = shape.y;
    float sagAmt = shape.z;

    uv -= p;
    uv = uv * 2.0 - 1.0;

    float d2 = spokes(uv, spikeFreq);

    uv.x *= 1.0 + uv.y * sagAmt;

    float d1 = dot(uv, uv);

    float threshold = 1.0;
    float d = d1 + d2 * spikeAmt;

    float f = 0.05;
    float c = smoothstep(threshold, threshold - f, d);

    return vec4(d1, d2, d, c);
}

vec4 ScaleCol(vec2 uv, vec2 p, vec4 i, vec2 scaleId) {
    float n = hash12(scaleId);
    vec3 lutColor = texture(gradientLUT, vec2(n, 0.5)).rgb;

    uv -= p;

    float grad = 1.0 - uv.y;
    float lum = (grad + i.x) * 0.2 + 0.5;

    vec3 spokeShade = lutColor * 0.3;
    vec3 edgeShade = lutColor * 1.5;

    vec4 c = vec4(lum * lutColor, i.a);
    c.rgb = mix(c.rgb, spokeShade, i.y * i.x * 0.5);
    c.rgb = mix(c.rgb, edgeShade, i.x);

    c = mix(vec4(0.0), c, i.a);

    float fade = 0.3;
    float shadow = smoothstep(1.0 + fade, 1.0, i.z);

    c.a = mix(shadow * 0.25, 1.0, i.a);

    return c;
}

vec4 Scale(vec2 uv, vec2 p, vec3 shape, vec2 scaleId) {
    vec4 info = ScaleInfo(uv, p, shape);
    vec4 c = ScaleCol(uv, p, info, scaleId);
    return c;
}

vec4 ScaleTex(vec2 uv, vec2 uv2, vec3 shape) {
    vec2 id = floor(uv2);
    uv2 -= id;

    vec4 rScale  = Scale(uv2, vec2( 0.5,  0.01), shape, id + vec2(1.0, 0.0));
    vec4 lScale  = Scale(uv2, vec2(-0.5,  0.01), shape, id + vec2(0.0, 0.0));
    vec4 bScale  = Scale(uv2, vec2( 0.0, -0.5 ), shape, id + vec2(0.0, 0.0));
    vec4 tScale  = Scale(uv2, vec2( 0.0,  0.5 ), shape, id + vec2(0.0, 1.0));
    vec4 t2Scale = Scale(uv2, vec2( 1.0,  0.5 ), shape, id + vec2(2.0, 1.0));

    rScale.rgb  *= brightness(uv, id + vec2(1.0, 0.0));
    lScale.rgb  *= brightness(uv, id + vec2(0.0, 0.0));
    bScale.rgb  *= brightness(uv, id + vec2(0.0, 0.0));
    tScale.rgb  *= brightness(uv, id + vec2(0.0, 1.0));
    t2Scale.rgb *= brightness(uv, id + vec2(2.0, 1.0));

    vec4 c = vec4(0.1, 0.3, 0.2, 1.0);
    c = mix(c, bScale,  bScale.a);
    c = mix(c, rScale,  rScale.a);
    c = mix(c, lScale,  lScale.a);
    c = mix(c, tScale,  tScale.a);
    c = mix(c, t2Scale, t2Scale.a);

    return c;
}

void main() {
    vec2 uv = fragTexCoord;
    uv.x *= resolution.x / resolution.y;
    uv.x += sin(wavePhase + uv.y * scaleSize) * waveAmplitude;

    uv -= 0.5;
    vec2 uv2 = uv * scaleSize;
    uv2.y -= scrollPhase;
    uv += 0.5;

    vec3 shape = vec3(spikeAmount, spikeFrequency, sag);
    vec4 c = ScaleTex(uv, uv2, shape);

    finalColor = vec4(c.rgb, 1.0);
}
