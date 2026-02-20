#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform int lanes;
uniform float cellWidth;
uniform float spacing;
uniform float gapSize;
uniform float scrollAngle;
uniform float scrollSpeed;
uniform float widthVariation;
uniform float colorMix;
uniform float jitter;
uniform float changeRate;
uniform float sparkIntensity;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform float flashProb;
uniform float flashIntensity;
uniform float breathProb;
uniform float breathRate;

float boxCov(float lo, float hi, float pc, float pw) {
    return clamp((min(hi, pc + pw) - max(lo, pc - pw)) / (2.0 * pw), 0.0, 1.0);
}

float h11(float p) { p = fract(p * 443.8975); p *= p + 33.33; return fract(p * p); }
float h21(vec2 p) { float n = dot(p, vec2(127.1, 311.7)); return fract(sin(n) * 43758.5453); }
float h31(vec3 p) { return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453); }

void main() {
    vec2 uv = fragTexCoord;
    float aspect = resolution.x / resolution.y;

    // Rotate UV space by scrollAngle (lanes run along rotated x-axis)
    vec2 center = vec2(0.5);
    vec2 p = uv - center;
    float ca = cos(scrollAngle), sa = sin(scrollAngle);
    vec2 ruv = vec2(ca * p.x + sa * p.y, -sa * p.x + ca * p.y) + center;

    // --- Lane identification ---
    float laneHeight = 1.0 / float(lanes);
    float laneF = ruv.y / laneHeight;
    int laneIdx = int(floor(laneF));
    float withinLane = fract(laneF);

    // Breathing: sinusoidal gap oscillation on selected lanes
    float breathSeed = h11(float(laneIdx) * 0.531 + 99.0);
    float effectiveGap = gapSize;
    if (breathSeed < breathProb) {
        float bRate = mix(0.15, 0.4, h11(float(laneIdx) * 0.831 + 111.0));
        float breath = sin(time * bRate * breathRate * 6.28318) * 0.5 + 0.5;
        effectiveGap = mix(0.02, 0.35, breath);
    }

    // Lane gap mask: dark separator between lanes
    float gapMask = smoothstep(0.0, effectiveGap * 0.5, withinLane)
                  * smoothstep(1.0, 1.0 - effectiveGap * 0.5, withinLane);

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
    float scrolledX = ruv.x * aspect + time * laneSpeed;

    // --- Cell identification ---
    float slotWidth = cellWidth * spacing;
    float cellIdxF = floor(scrolledX / slotWidth);

    vec3 color = vec3(0.0);

    // Widen loop to [-2, +2] for spark detection between adjacent pairs
    const int N = 5;
    float cellCenters[N];
    float cellHalfWs[N];
    vec3 cellColors[N];
    bool cellIsColor[N];

    for (int dc = -2; dc <= 2; dc++) {
        int i = dc + 2;
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

        // Store for spark pass
        cellCenters[i] = jitteredCenter;
        cellHalfWs[i] = halfW;

        // Pixel half-width for analytical AA
        float pw = 0.5 * aspect / resolution.x;

        // Coverage
        float covX = boxCov(jitteredCenter - halfW, jitteredCenter + halfW, scrolledX, pw);
        float covY = gapMask;
        float cov = covX * covY;

        // Determine cell color and type (needed for both rendering and sparks)
        float t = h21(vec2(idx + 0.1, float(laneIdx) + 0.2));
        float colorGate = h21(vec2(idx * 1.7, float(laneIdx) * 3.1));
        bool isColorCell = colorGate < colorMix;
        vec3 cCol = vec3(0.0);

        if (isColorCell) {
            cCol = texture(gradientLUT, vec2(t, 0.5)).rgb;
            float freq = baseFreq * pow(maxFreq / baseFreq, t);
            float bin = freq / (sampleRate * 0.5);
            float mag = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
            mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
            float brightness = baseBright + mag;
            cCol *= brightness;
        } else {
            float gray = 0.03 + 0.02 * h11(idx * 13.7 + float(laneIdx));
            cCol = vec3(gray);
        }

        cellColors[i] = cCol;
        cellIsColor[i] = isColorCell;

        if (cov > 0.001) {
            color += cCol * cov;
        }
    }

    // --- Spark detection between adjacent cells ---
    float sparkThresh = slotWidth * 0.7;
    float sparkOverlap = slotWidth * -0.5;
    if (sparkIntensity > 0.0 && gapMask > 0.01) {
        for (int i = 1; i < N; i++) {
            float rightEdge = cellCenters[i - 1] + cellHalfWs[i - 1];
            float leftEdge = cellCenters[i] - cellHalfWs[i];
            float gap = leftEdge - rightEdge;

            // Spark when cells are close or overlapping
            if (gap < sparkThresh && gap > sparkOverlap) {
                float gapCenter = (rightEdge + leftEdge) * 0.5;
                float gapDist = abs(scrolledX - gapCenter);
                float sparkWidth = slotWidth * 0.15 + slotWidth * 0.1 * (1.0 - gap / sparkThresh);

                if (gapDist < sparkWidth) {
                    float proximity = max(0.0, 1.0 - gap / sparkThresh);
                    float spark = (1.0 - gapDist / sparkWidth) * proximity;
                    spark *= spark;

                    // Spark color from neighboring cells
                    vec3 sparkCol;
                    if (cellIsColor[i - 1] || cellIsColor[i]) {
                        sparkCol = (cellIsColor[i - 1] && cellIsColor[i])
                            ? (cellColors[i - 1] + cellColors[i]) * 0.5
                            : (cellIsColor[i - 1] ? cellColors[i - 1] : cellColors[i]);
                    } else {
                        sparkCol = vec3(0.4, 0.6, 1.0);
                    }

                    // Overlapping cells get brighter, plus rapid flicker
                    float idx = cellIdxF + float(i - 2);
                    float boost = (gap < 0.0) ? 2.0 : 1.0;
                    boost *= 0.7 + 0.6 * h31(vec3(idx, float(laneIdx), floor(time * 12.0)));

                    color += sparkCol * spark * boost * sparkIntensity * gapMask;
                }
            }
        }
    }

    // Row flash: probabilistic full-brightness accent
    float rfl = h21(vec2(float(laneIdx), floor(time * 1.5)));
    if (rfl > (1.0 - flashProb)) {
        color = mix(color, vec3(1.0), flashIntensity);
    }

    finalColor = vec4(color, 1.0);
}
