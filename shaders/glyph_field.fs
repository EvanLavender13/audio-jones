// Glyph field — layered scrolling character grids as abstract texture
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D fontAtlas;
uniform sampler2D gradientLUT;
uniform vec2 resolution;

// Grid and layers
uniform float gridSize;
uniform int layerCount;
uniform float layerScaleSpread;
uniform float layerSpeedSpread;
uniform float layerOpacity;

// Scroll
uniform int scrollDirection;
uniform float scrollTime; // CPU-accumulated: scrollSpeed * dt

// Flutter
uniform float flutterAmount;
uniform float flutterTime; // CPU-accumulated: flutterSpeed * dt

// Wave
uniform float waveAmplitude;
uniform float waveFreq;
uniform float waveTime; // CPU-accumulated: waveSpeed * dt

// Drift
uniform float driftAmount;
uniform float driftTime; // CPU-accumulated: driftSpeed * dt

// Stutter
uniform float stutterAmount;
uniform float stutterTime; // CPU-accumulated: stutterSpeed * dt
uniform float stutterDiscrete;

// Effects
uniform float bandDistortion;
uniform float inversionRate;
uniform float inversionTime; // CPU-accumulated: inversionSpeed * dt
uniform int lcdMode;
uniform float lcdFreq;

// FFT reactivity
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform int numOctaves;
uniform float gain;
uniform float curve;
uniform float baseBright;

// --- PCG3D stateless hash ---

uvec3 pcg3d(uvec3 v) {
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
    return v;
}

vec3 pcg3df(vec3 p) {
    return vec3(pcg3d(floatBitsToUint(p))) / float(0xFFFFFFFFu);
}

// --- Font atlas sampling ---
// 16x16 grid of glyphs, white on black (shape in RGB)
vec4 sampleGlyph(int charIdx, vec2 localUV) {
    int cx = charIdx & 15;
    int cy = charIdx >> 4;
    vec2 cellOrigin = vec2(float(cx), float(cy)) / 16.0;
    vec2 charUV = cellOrigin + localUV / 16.0;
    return texture(fontAtlas, charUV);
}

void main() {
    // Center UV at screen origin so layers scale from center
    vec2 uv = fragTexCoord - 0.5;
    float aspect = resolution.x / resolution.y;
    uv.x *= aspect;

    // Per-pixel noise for soft inversion transitions
    vec3 pixelNoise = pcg3df(vec3(fragTexCoord * resolution, 0.0));

    vec3 total = vec3(0.0);

    for (int layer = 0; layer < layerCount; layer++) {
        float layerF = float(layer);
        float scale = pow(layerScaleSpread, layerF);
        float speed = pow(layerSpeedSpread, layerF);
        float opacity = pow(layerOpacity, layerF);

        float gs = gridSize * scale;

        // Wave displacement before grid quantization
        vec2 wUV = uv;
        wUV.x += sin(uv.y * waveFreq + waveTime + layerF * 1.7) * waveAmplitude;
        wUV.y += cos(uv.x * waveFreq + waveTime + layerF * 2.3) * waveAmplitude;

        // Band distortion: rows near center normal, edges compress
        float distFromCenter = abs(wUV.y);
        float bandScale = 1.0 + distFromCenter * bandDistortion * 2.0;
        vec2 gridUV = wUV;
        gridUV.y = wUV.y * bandScale;

        // Drift: per-cell position wander before cell quantization
        vec2 prelimCell = floor(gridUV * gs);
        float driftFloor = floor(driftTime + layerF * 3.1);
        float driftFrac = smoothstep(0.0, 1.0, fract(driftTime + layerF * 3.1));
        vec3 dh0 = pcg3df(vec3(prelimCell, driftFloor));
        vec3 dh1 = pcg3df(vec3(prelimCell, driftFloor + 1.0));
        vec2 drift = mix(dh0.xy, dh1.xy, driftFrac) - 0.5;
        gridUV += drift * driftAmount;

        // Grid cell computation
        vec2 cellCoord = floor(gridUV * gs);
        vec2 localUV = fract(gridUV * gs);

        // Scroll offset based on direction mode
        if (scrollDirection == 0) {
            // Horizontal: per-row speed variation
            float rowHash = pcg3df(vec3(cellCoord.y, layerF, 31.7)).x;
            float scrollOffset = scrollTime * (rowHash - 0.5) * speed;
            // Stutter: per-lane freeze gate
            float laneHash = pcg3df(vec3(cellCoord.y, layerF, 91.3)).x;
            float stutterStep = floor(stutterTime * (0.5 + laneHash));
            float gate = step(0.5, sin(stutterStep));
            float mask = mix(1.0, gate, stutterAmount);
            scrollOffset *= mask;
            float offsetX = gridUV.x + scrollOffset;
            // Discrete quantization: snap to cell boundaries
            float qOffsetX = floor(offsetX * gs) / gs;
            offsetX = mix(offsetX, qOffsetX, stutterDiscrete);
            cellCoord.x = floor(offsetX * gs);
            localUV.x = fract(offsetX * gs);
        } else if (scrollDirection == 1) {
            // Vertical: per-column speed variation
            float colHash = pcg3df(vec3(cellCoord.x, layerF, 47.3)).x;
            float scrollOffset = scrollTime * (colHash - 0.5) * speed;
            // Stutter: per-lane freeze gate
            float laneHash = pcg3df(vec3(cellCoord.x, layerF, 91.3)).x;
            float stutterStep = floor(stutterTime * (0.5 + laneHash));
            float gate = step(0.5, sin(stutterStep));
            float mask = mix(1.0, gate, stutterAmount);
            scrollOffset *= mask;
            float offsetY = gridUV.y + scrollOffset;
            // Discrete quantization: snap to cell boundaries
            float qOffsetY = floor(offsetY * gs) / gs;
            offsetY = mix(offsetY, qOffsetY, stutterDiscrete);
            cellCoord.y = floor(offsetY * gs);
            localUV.y = fract(offsetY * gs);
        } else {
            // Radial: polar coordinates with per-ring angular scroll
            vec2 centered = gridUV;
            float r = length(centered);
            float theta = atan(centered.y, centered.x) / 6.2831853 + 0.5;
            float ringIdx = floor(r * gs);
            float ringHash = pcg3df(vec3(ringIdx, layerF, 63.1)).x;
            float angularOffset = scrollTime * (ringHash - 0.5) * speed;
            // Stutter: per-lane freeze gate
            float laneHash = pcg3df(vec3(ringIdx, layerF, 91.3)).x;
            float stutterStep = floor(stutterTime * (0.5 + laneHash));
            float gate = step(0.5, sin(stutterStep));
            float mask = mix(1.0, gate, stutterAmount);
            angularOffset *= mask;
            theta = fract(theta + angularOffset);
            // Discrete quantization: snap to cell boundaries
            float qTheta = floor(theta * gs) / gs;
            theta = mix(theta, qTheta, stutterDiscrete);
            cellCoord = vec2(floor(theta * gs), ringIdx);
            localUV = vec2(fract(theta * gs), fract(r * gs));
        }

        // Per-cell hash after scroll repositioning
        vec3 h = pcg3df(vec3(cellCoord, layerF));
        vec3 h2 = pcg3df(vec3(cellCoord + 100.0, layerF + 7.0));

        // Character index with flutter — multiply by prime for visible cycling
        float baseChar = h.x * 255.0;
        float flutterStep = floor(flutterTime + h.y * 100.0);
        int charIdx = int(mod(baseChar + flutterStep * 37.0 * flutterAmount, 256.0));

        // Sample font atlas — glyph shapes in RGB (white on black)
        vec2 clampedUV = clamp(localUV, 0.0, 1.0);
        float glyphAlpha = sampleGlyph(charIdx, clampedUV).r;

        // Inversion flicker with per-pixel dissolve transition
        // pixelNoise adds sub-cell variation so inversions sweep rather than snap
        float invFlicker = fract(h2.x + inversionTime + pixelNoise.z * 0.01);
        if (invFlicker < inversionRate) {
            glyphAlpha = 1.0 - glyphAlpha;
        }

        // FFT per-cell — each cell hashes to a semitone across full range
        // Color and reactivity share the same mapping: same color = same frequency
        int totalSemitones = numOctaves * 12;
        int globalSemi = int(floor(h.z * float(totalSemitones)));
        float lutPos = (float(globalSemi) + 0.5) / float(totalSemitones);

        // FFT lookup — semitone to frequency to normalized bin
        float freq = baseFreq * pow(2.0, float(globalSemi) / 12.0);
        float bin = freq / (sampleRate * 0.5);
        float mag = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
        mag = pow(clamp(mag * gain, 0.0, 1.0), curve);

        // Color from gradient LUT — position matches semitone, so color = frequency
        vec3 glyphColor = textureLod(gradientLUT, vec2(lutPos, 0.5), 0.0).rgb;

        // Brightness: baseBright is the floor, mag adds on top
        float brightness = baseBright + (1.0 - baseBright) * mag;

        // Accumulate layer — opacity and brightness are independent
        total += glyphColor * glyphAlpha * opacity * brightness;
    }

    // LCD sub-pixel: 2D pixel grid from reference (sin(x+phase)*sin(y)), brightness compensated
    if (lcdMode != 0) {
        vec2 pixelCoord = fragTexCoord * resolution;
        total *= max(sin(pixelCoord.x * lcdFreq + vec3(0.0, 2.0, 4.0))
                   * sin(pixelCoord.y * lcdFreq), 0.0) * 3.0;
    }

    finalColor = vec4(total, 1.0);
}
