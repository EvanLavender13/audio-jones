// Infinity Matrix - recursive fractal glyph zoom with gradient LUT and FFT audio
// Based on "Infinity Matrix" by KilledByAPixel (Frank Force)
// https://www.shadertoy.com/view/Md2fRR
// License: CC BY-NC-SA 3.0
// Modified: uniforms for recursion/fade, gradient LUT replaces HSV,
// FFT audio, wave distortion, colorFreqMap toggle

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform sampler2D gradientLUT;
uniform float zoomPhase;
uniform float zoomScale;
uniform int recursionDepth;
uniform float fadeDepth;
uniform float waveAmplitude;
uniform float waveFreq;
uniform float wavePhase;
uniform int colorFreqMap;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const int glyphSize = 5;
const int glyphCount = 2;
const float glyphMargin = 0.5;
const float glyphSizeF = float(glyphSize) + 2.0 * glyphMargin;
const float glyphSizeLog = log(glyphSizeF);
const int powTableCount = 10;
const float gsfi = 1.0 / glyphSizeF;
const float powTable[powTableCount] = float[](
    1.0, gsfi, pow(gsfi, 2.0), pow(gsfi, 3.0), pow(gsfi, 4.0),
    pow(gsfi, 5.0), pow(gsfi, 6.0), pow(gsfi, 7.0), pow(gsfi, 8.0), pow(gsfi, 9.0)
);
const float E = 2.718281828459;

const int glyphs[glyphSize * glyphCount] = int[](
    0x01110, 0x01110,
    0x11011, 0x11110,
    0x11011, 0x01110,
    0x11011, 0x01110,
    0x01110, 0x11111
);

float RandFloat(int i) { return fract(sin(float(i)) * 43758.5453); }
int RandInt(int i) { return int(100000.0 * RandFloat(i)); }

int GetFocusGlyph(int i) { return RandInt(i) % glyphCount; }
int GetGlyphPixelRow(int y, int g) { return glyphs[g + (glyphSize - 1 - y) * glyphCount]; }

int GetGlyphPixel(ivec2 pos, int g) {
    if (pos.x >= glyphSize || pos.y >= glyphSize) { return 0; }
    int glyphRow = GetGlyphPixelRow(pos.y, g);
    return 1 & (glyphRow >> (glyphSize - 1 - pos.x) * 4);
}

ivec2 focusList[max(powTableCount, 8) + 2]; // 8 = max recursionDepth uniform
ivec2 GetFocusPos(int i) { return focusList[i + 2]; }

ivec2 CalculateFocusPos(int iterations) {
    int g = GetFocusGlyph(iterations - 1);
    int c = 18; // both glyphs have 18 lit pixels
    c -= RandInt(iterations) % c;
    for (int y = glyphCount * (glyphSize - 1); y >= 0; y -= glyphCount) {
        int glyphRow = glyphs[g + y];
        for (int x = 0; x < glyphSize; ++x) {
            c -= (1 & (glyphRow >> 4 * x));
            if (c == 0) {
                return ivec2(glyphSize - 1 - x, glyphSize - 1 - y / glyphCount);
            }
        }
    }
    return ivec2(0); // unreachable
}

int GetGlyph(int iterations, ivec2 glyphPos, int glyphLast,
             ivec2 glyphPosLast, ivec2 focusPos) {
    if (glyphPos == focusPos) { return GetFocusGlyph(iterations); }
    int seed = iterations + glyphPos.x * 313 + glyphPos.y * 411
             + glyphPosLast.x * 557 + glyphPosLast.y * 121;
    return RandInt(seed) % glyphCount;
}

float GetRecursionFade(int r, float timePercent) {
    if (r > recursionDepth) { return timePercent; }
    float rt = max(float(r) - timePercent - fadeDepth, 0.0);
    float rc = float(recursionDepth) - fadeDepth;
    return rt / rc;
}

vec3 GetPixelFractal(vec2 pos, int iterations, float timePercent) {
    int glyphLast = GetFocusGlyph(iterations - 1);
    ivec2 glyphPosLast = GetFocusPos(-2);
    ivec2 glyphPos = GetFocusPos(-1);

    bool isFocus = true;
    ivec2 focusPos = glyphPos;

    vec3 color = vec3(0.0);
    for (int r = 0; r <= recursionDepth + 1; ++r) {
        float t;
        if (colorFreqMap == 0) {
            // Depth mode: recursion level drives color and FFT
            t = float(r) / float(recursionDepth);
        } else {
            // Hash mode: glyph position hash drives color and FFT
            int seed = iterations + r + 11 * glyphPosLast.x + 13 * glyphPosLast.y
                     + 17 * glyphPos.x + 19 * glyphPos.y;
            t = RandFloat(seed);
        }
        vec3 lutColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

        float freq = baseFreq * pow(maxFreq / baseFreq, t);
        float bin = freq / (sampleRate * 0.5);
        float energy = 0.0;
        if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
        float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        float f = GetRecursionFade(r, timePercent);
        color += lutColor * brightness * f;

        if (r > recursionDepth) { return color; }

        // Update pos
        pos -= vec2(glyphMargin * gsfi);
        pos *= glyphSizeF;

        glyphPosLast = glyphPos;
        glyphPos = ivec2(pos);

        int glyphValue = GetGlyphPixel(glyphPos, glyphLast);
        if (glyphValue == 0 || pos.x < 0.0 || pos.y < 0.0) { return color; }

        pos -= vec2(floor(pos));
        focusPos = isFocus ? GetFocusPos(r) : ivec2(-10);
        glyphLast = GetGlyph(iterations + r, glyphPos, glyphLast, glyphPosLast, focusPos);
        isFocus = isFocus && (glyphPos == focusPos);
    }
    return color; // loop always returns early, but compiler needs this
}

void main() {
    // Center UV, normalize by height
    vec2 uv = fragTexCoord * resolution - resolution * 0.5;
    uv /= resolution.y;

    // Wave distortion (replaces InitUV)
    uv.x += waveAmplitude * sin(waveFreq * uv.y + wavePhase);
    uv.y += waveAmplitude * sin(waveFreq * uv.x + 0.8 * wavePhase);

    // zoomPhase accumulated on CPU (replaces iTime * zoomSpeed)
    float timePercent = zoomPhase;
    int iterations = int(floor(timePercent));
    timePercent -= float(iterations);

    // Zoom
    float zoom = pow(E, -glyphSizeLog * timePercent) * zoomScale;

    // Cache focus positions
    for (int i = 0; i < powTableCount + 2; ++i) {
        focusList[i] = CalculateFocusPos(iterations + i - 2);
    }

    // Offset
    vec2 offset = vec2(0.0);
    for (int i = 0; i < powTableCount; ++i) {
        offset += ((vec2(GetFocusPos(i)) + vec2(glyphMargin)) * gsfi) * powTable[i];
    }

    // Apply zoom & offset
    vec2 uvFractal = uv * zoom + offset;

    vec3 pixelColor = GetPixelFractal(uvFractal, iterations, timePercent);

    finalColor = vec4(pixelColor, 1.0);
}
