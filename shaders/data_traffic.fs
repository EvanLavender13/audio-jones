#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform int lanes;
uniform float cellWidth;
uniform float spacing;
uniform float gapSize;
uniform float scrollSpeed;
uniform float widthVariation;
uniform float colorMix;
uniform float jitter;
uniform float changeRate;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

float boxCov(float lo, float hi, float pc, float pw) {
    return clamp((min(hi, pc + pw) - max(lo, pc - pw)) / (2.0 * pw), 0.0, 1.0);
}

float h11(float p) { p = fract(p * 443.8975); p *= p + 33.33; return fract(p * p); }
float h21(vec2 p) { float n = dot(p, vec2(127.1, 311.7)); return fract(sin(n) * 43758.5453); }

void main() {
    vec2 uv = fragTexCoord;
    float aspect = resolution.x / resolution.y;

    // --- Lane identification ---
    float laneHeight = 1.0 / float(lanes);
    float laneF = uv.y / laneHeight;
    int laneIdx = int(floor(laneF));
    float withinLane = fract(laneF);

    // Lane gap mask: dark separator between lanes
    float gapMask = smoothstep(0.0, gapSize * 0.5, withinLane)
                  * smoothstep(1.0, 1.0 - gapSize * 0.5, withinLane);

    // --- Per-lane scroll ---
    float laneHash = h11(float(laneIdx) * 7.31);
    float laneDir = (laneHash > 0.5) ? 1.0 : -1.0;
    float laneBaseSpeed = 0.3 + laneHash * 0.7;

    // Epoch-based speed variation
    float speedEpoch = floor(time * changeRate + laneHash * 100.0);
    float speedProgress = fract(time * changeRate + laneHash * 100.0);
    float epochSpeedMul = mix(
        h21(vec2(speedEpoch, float(laneIdx) + 0.5)),
        h21(vec2(speedEpoch + 1.0, float(laneIdx) + 0.5)),
        smoothstep(0.0, 1.0, speedProgress)
    );
    epochSpeedMul = (epochSpeedMul < 0.2) ? epochSpeedMul * 0.1 : epochSpeedMul;

    float laneSpeed = laneDir * laneBaseSpeed * epochSpeedMul * scrollSpeed;
    float scrolledX = uv.x * aspect + time * laneSpeed;

    // --- Cell identification ---
    float slotWidth = cellWidth * spacing;
    float cellIdxF = floor(scrolledX / slotWidth);

    vec3 color = vec3(0.0);
    float totalCov = 0.0;

    // Check current cell and neighbors for coverage
    for (int dc = -1; dc <= 1; dc++) {
        float idx = cellIdxF + float(dc);
        float cellCenter = (idx + 0.5) * slotWidth;

        // Epoch-based cell width variation
        float widthEpoch = floor(time * changeRate + h21(vec2(idx, float(laneIdx))) * 100.0);
        float widthProgress = fract(time * changeRate + h21(vec2(idx, float(laneIdx))) * 100.0);
        float widthRand = mix(
            h21(vec2(widthEpoch + idx, float(laneIdx) + 1.0)),
            h21(vec2(widthEpoch + idx + 1.0, float(laneIdx) + 1.0)),
            smoothstep(0.0, 1.0, widthProgress)
        );
        float halfW = cellWidth * 0.5 * (1.0 - widthVariation + widthVariation * widthRand);

        // Jitter
        float jitterOffset = (h21(vec2(idx * 3.7, float(laneIdx) * 2.3 + widthEpoch)) - 0.5) * jitter * cellWidth;
        float jitteredCenter = cellCenter + jitterOffset;

        // Pixel half-width for analytical AA
        float pw = 0.5 * aspect / resolution.x;

        // Coverage
        float covX = boxCov(jitteredCenter - halfW, jitteredCenter + halfW, scrolledX, pw);
        float covY = gapMask;
        float cov = covX * covY;

        if (cov > 0.001) {
            float t = h21(vec2(idx + 0.1, float(laneIdx) + 0.2));
            float colorGate = h21(vec2(idx * 1.7, float(laneIdx) * 3.1));

            if (colorGate < colorMix) {
                // Colored cell: LUT color + FFT brightness
                vec3 cellColor = texture(gradientLUT, vec2(t, 0.5)).rgb;
                float freq = baseFreq * pow(maxFreq / baseFreq, t);
                float bin = freq / (sampleRate * 0.5);
                float mag = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
                mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
                float brightness = baseBright + mag;
                color += cellColor * cov * brightness;
            } else {
                // Grayscale filler
                float gray = 0.03 + 0.02 * h11(idx * 13.7 + float(laneIdx));
                color += vec3(gray) * cov;
            }
            totalCov += cov;
        }
    }

    finalColor = vec4(color, 1.0);
}
