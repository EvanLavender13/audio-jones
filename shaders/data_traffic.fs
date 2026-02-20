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
uniform float breathProb;
uniform float breathPhase;
uniform float glowIntensity;
uniform float glowRadius;
uniform float twitchProb;
uniform float twitchIntensity;
uniform float splitProb;
uniform float mergeProb;
uniform float fissionProb;
uniform float phaseShiftProb;
uniform float phaseShiftIntensity;
uniform float springProb;
uniform float springIntensity;
uniform float widthSpringProb;
uniform float widthSpringIntensity;

float boxCov(float lo, float hi, float pc, float pw) {
    return clamp((min(hi, pc + pw) - max(lo, pc - pw)) / (2.0 * pw), 0.0, 1.0);
}

float h11(float p) { p = fract(p * 443.8975); p *= p + 33.33; return fract(p * p); }
float h21(vec2 p) { float n = dot(p, vec2(127.1, 311.7)); return fract(sin(n) * 43758.5453); }
float h31(vec3 p) { return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453); }

float laneScrolledX(float laneF, float xCoord) {
    float lh = h11(laneF * 7.31);
    float dir = (lh > 0.5) ? 1.0 : -1.0;
    float baseSpd = 0.3 + lh * 0.7;
    float sEpoch = floor(time * changeRate + lh * 100.0);
    float sProgress = fract(time * changeRate + lh * 100.0);
    float eMul = mix(
        h21(vec2(sEpoch, laneF + 0.5)),
        h21(vec2(sEpoch + 1.0, laneF + 0.5)),
        smoothstep(0.0, 1.0, sProgress)
    );
    eMul = (eMul < 0.2) ? eMul * 0.1 : eMul;
    float aspect = resolution.x / resolution.y;
    float scrolledX = xCoord * aspect + time * dir * baseSpd * eMul * scrollSpeed;

    float phaseSeed = h11(laneF * 0.371 + 222.0);
    if (phaseSeed < phaseShiftProb) {
        float phaseRate = mix(0.08, 0.2, h11(laneF * 0.621 + 333.0));
        float phaseEpoch = floor(time * phaseRate + phaseSeed * 20.0);
        float phaseT = fract(time * phaseRate + phaseSeed * 20.0);
        float phaseDir = h11(laneF * 0.441 + phaseEpoch * 7.7) > 0.5 ? 1.0 : -1.0;
        float phaseKick = 0.0;
        if (phaseT < 0.15) {
            float snapT2 = phaseT / 0.15;
            phaseKick = phaseDir * 8.0 * (1.0 - snapT2);
        } else if (phaseT < 0.6) {
            float wobT = (phaseT - 0.15) / 0.45;
            phaseKick = phaseDir * 3.0 * exp(-4.0 * wobT) * cos(wobT * 40.0);
        }
        scrolledX += phaseKick * phaseShiftIntensity * cellWidth * spacing;
    }

    return scrolledX;
}

float epochBrightness(float idx, float laneF) {
    float s1 = h21(vec2(idx, laneF));
    float s3 = h21(vec2(idx + 200.0, laneF));
    float rh6 = h11(laneF * 0.4217 + 57.3);
    float bEpoch = floor(time * 0.6 + s1 * 20.0);
    float bs = h31(vec3(idx, laneF, bEpoch));
    bs = mix(bs, rh6, 0.3);
    if (bs < 0.25) return 0.0;
    if (bs < 0.45) return mix(0.05, 0.2, s3);
    if (bs < 0.7) return mix(0.25, 0.55, s3);
    return mix(0.65, 1.0, s3);
}

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
        float breath = sin(breathPhase * bRate * 6.28318) * 0.5 + 0.5;
        effectiveGap = mix(0.02, 0.35, breath);
    }

    // Lane gap mask: dark separator between lanes
    float gapMask = smoothstep(0.0, effectiveGap * 0.5, withinLane)
                  * smoothstep(1.0, 1.0 - effectiveGap * 0.5, withinLane);

    // --- Per-lane scroll ---
    float scrolledX = laneScrolledX(float(laneIdx), ruv.x);

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

        // Split — momentary width shrink
        float spS = h21(vec2(idx + widthEpoch * 11.3, float(laneIdx) + 77.0));
        if (spS < splitProb) {
            float p = fract(time * changeRate + h21(vec2(idx + 200.0, float(laneIdx))) * 12.0);
            float c = smoothstep(0.0, 0.1, p) * (1.0 - smoothstep(0.5, 0.8, p));
            halfW *= mix(1.0, 0.05, c);
        }
        // Merge — momentary width expand
        float mS = h21(vec2(idx + widthEpoch * 5.7, float(laneIdx) + 33.0));
        if (mS < mergeProb) {
            float p = fract(time * changeRate + h21(vec2(idx + 200.0, float(laneIdx))) * 12.0);
            float e2 = smoothstep(0.0, 0.2, p) * (1.0 - smoothstep(0.6, 0.9, p));
            halfW *= mix(1.0, 2.5, e2);
        }

        // Doorstop spring width: decaying width oscillation
        float springWSeed = h21(vec2(idx * 3.3, float(laneIdx) + 155.0));
        if (springWSeed < widthSpringProb) {
            float swRate = mix(0.15, 0.4, h21(vec2(idx + 900.0, float(laneIdx))));
            float s4 = h21(vec2(idx + 400.0, float(laneIdx)));
            float swEpoch = floor(time * swRate + s4 * 8.0);
            float swT = fract(time * swRate + s4 * 8.0);
            float wKick = mix(0.3, 3.0, h21(vec2(idx + swEpoch * 7.7, float(laneIdx) + 111.0)));
            float wDecay = exp(-3.5 * swT);
            float wWobble = cos(swT * 40.0);
            float wMul = 1.0 + (wKick - 1.0) * wDecay * wWobble * widthSpringIntensity;
            halfW *= max(wMul, 0.02);
        }

        // Jitter
        float jitterOffset = (h21(vec2(idx * 3.7, float(laneIdx) * 2.3 + widthEpoch)) - 0.5) * jitter * cellWidth;
        float jitteredCenter = cellCenter + jitterOffset;

        // Per-cell twitch: rapid oscillation bursts
        float twitchSeed = h21(vec2(idx * 7.3, float(laneIdx) * 3.1 + 55.0));
        if (twitchSeed < twitchProb) {
            float twitchRate = mix(8.0, 25.0, h21(vec2(idx + 500.0, float(laneIdx))));
            float twitchAmp = slotWidth * mix(0.08, 0.25, h21(vec2(idx + 600.0, float(laneIdx)))) * twitchIntensity;
            float twitch = sin(time * twitchRate + idx * 3.7) * 0.6
                         + sin(time * twitchRate * 1.7 + idx * 5.1) * 0.3
                         + sin(time * twitchRate * 3.1 + idx * 9.3) * 0.1;
            float burstSeed = h21(vec2(idx + 700.0, float(laneIdx)));
            float burst = smoothstep(0.3, 0.5, sin(time * mix(0.3, 0.8, burstSeed) + burstSeed * 6.28));
            jitteredCenter += twitch * twitchAmp * burst;
        }

        // Doorstop spring position: decaying oscillation toward random target
        float springChance = h21(vec2(idx * 4.1, float(laneIdx) + 123.0));
        if (springChance < springProb) {
            float springRate = mix(0.15, 0.5, h21(vec2(idx + 800.0, float(laneIdx))));
            float s2 = h21(vec2(idx + 350.0, float(laneIdx)));
            float springEpoch = floor(time * springRate + s2 * 10.0);
            float springT = fract(time * springRate + s2 * 10.0);
            float target = (h21(vec2(idx + springEpoch * 5.1, float(laneIdx) + 90.0)) - 0.5) * slotWidth * 5.0 * springIntensity;
            float kick = exp(-3.0 * springT);
            float wobble = cos(springT * 50.0);
            jitteredCenter += target * kick * wobble;
        }

        // Fission — cell divides into two visible sub-cells
        float fissionPhase = 0.0;
        float fissionDrift = 0.0;
        float fiS = h21(vec2(idx * 6.6, float(laneIdx) + 345.0));
        if (fiS < fissionProb) {
            float fissionRate = mix(0.1, 0.3, h21(vec2(idx * 0.4217, float(laneIdx) + 57.3)));
            float fissionSeed = h21(vec2(idx + 300.0, float(laneIdx)));
            float fissionEpoch = floor(time * fissionRate + fissionSeed * 15.0);
            float fissionT = fract(time * fissionRate + fissionSeed * 15.0);
            float fissionActive = h21(vec2(idx + fissionEpoch * 9.9, float(laneIdx) + 456.0));
            if (fissionActive < 0.5) {
                if (fissionT < 0.2) fissionPhase = smoothstep(0.0, 0.2, fissionT);
                else if (fissionT < 0.7) fissionPhase = 1.0;
                else fissionPhase = 1.0 - smoothstep(0.7, 0.9, fissionT);
                fissionDrift = fissionPhase * slotWidth * 0.4;
            }
        }

        // Store for spark pass (use post-fission geometry)
        cellCenters[i] = jitteredCenter;
        if (fissionPhase > 0.0) {
            float subHW = halfW * mix(1.0, 0.45, fissionPhase);
            cellHalfWs[i] = fissionDrift + subHW;
        } else {
            cellHalfWs[i] = halfW;
        }

        // Pixel half-width for analytical AA
        float pw = 0.5 * aspect / resolution.x;

        // Coverage — two sub-cells when fissioning, one otherwise
        float covX;
        if (fissionPhase > 0.0) {
            float subHW = halfW * mix(1.0, 0.45, fissionPhase);
            covX = max(
                boxCov(jitteredCenter - fissionDrift - subHW, jitteredCenter - fissionDrift + subHW, scrolledX, pw),
                boxCov(jitteredCenter + fissionDrift - subHW, jitteredCenter + fissionDrift + subHW, scrolledX, pw)
            );
        } else {
            covX = boxCov(jitteredCenter - halfW, jitteredCenter + halfW, scrolledX, pw);
        }
        float covY = gapMask;
        float cov = covX * covY;

        // Determine cell color and type (needed for both rendering and sparks)
        float t = h21(vec2(idx + 0.1, float(laneIdx) + 0.2));
        float colorGate = h21(vec2(idx * 1.7, float(laneIdx) * 3.1));
        bool isColorCell = colorGate < colorMix;
        vec3 cCol = vec3(0.0);

        // Epoch-based brightness (all cells)
        float brightness = epochBrightness(idx, float(laneIdx));
        // 3% full-flash chance
        float fl = h31(vec3(idx, float(laneIdx), floor(time * 5.0)));
        if (fl > 0.97) brightness = 1.0;

        if (isColorCell) {
            cCol = texture(gradientLUT, vec2(t, 0.5)).rgb;
            float freq = baseFreq * pow(maxFreq / baseFreq, t);
            float bin = freq / (sampleRate * 0.5);
            float mag = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
            mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
            cCol *= brightness * (baseBright + mag);
        } else {
            cCol = vec3(brightness);
            if (brightness > 0.5)
                cCol = mix(cCol, vec3(brightness * 0.92, brightness * 0.96, brightness), 0.15);
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

    // --- Proximity glow: soft colored light from bright/colored cells ---
    vec3 glow = vec3(0.0);
    if (glowIntensity > 0.0) {
        float lhAspect = laneHeight * aspect;
        float gr = cellWidth * glowRadius;
        float gr2inv = 1.0 / (gr * gr * 0.5);
        float gSlotW = cellWidth * spacing;

        for (int rowOff = -1; rowOff <= 1; rowOff++) {
            int gLane = laneIdx + rowOff;
            if (gLane < 0 || gLane >= lanes) continue;
            float gLaneF = float(gLane);

            float gScrollX = laneScrolledX(gLaneF, ruv.x);
            float gCellIdx = floor(gScrollX / gSlotW);

            float dy = float(abs(rowOff)) * lhAspect;
            float dy2 = dy * dy;

            for (int di = -3; di <= 3; di++) {
                float gIdx = gCellIdx + float(di);
                float gCenter = (gIdx + 0.5) * gSlotW;

                // Jitter
                float gWE = floor(time * changeRate + h21(vec2(gIdx, gLaneF)) * 100.0);
                gCenter += (h21(vec2(gIdx * 3.7, gLaneF * 2.3 + gWE)) - 0.5) * jitter * cellWidth;

                float gBright = epochBrightness(gIdx, gLaneF);

                // Emit check: color cell or bright enough
                float gColorGate = h21(vec2(gIdx * 1.7, gLaneF * 3.1));
                bool gIsColor = gColorGate < colorMix;
                if (!gIsColor && gBright <= 0.4) continue;

                // Glow color
                vec3 gCol;
                if (gIsColor) {
                    float gt = h21(vec2(gIdx + 0.1, gLaneF + 0.2));
                    gCol = texture(gradientLUT, vec2(gt, 0.5)).rgb * max(gBright, 0.5);
                } else {
                    gCol = vec3(gBright);
                }

                // Gaussian falloff
                float dx = abs(gScrollX - gCenter);
                float d2 = dx * dx + dy2;
                glow += gCol * exp(-d2 * gr2inv) * 0.45;
            }
        }
        glow *= glowIntensity;
    }

    finalColor = vec4(color + glow, 1.0);
}
