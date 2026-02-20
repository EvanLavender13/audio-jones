# Data Traffic — Behavior Catalog

Catalog of cell-level animation behaviors from the Shadertoy reference shader, ordered by implementation complexity. Each behavior modifies cell position (`center`), cell width (`hw`), brightness, or color within the existing per-cell loop. All are individually toggleable via a probability parameter (what fraction of cells exhibit the behavior) and an intensity parameter.

## Classification

- **Category**: GENERATORS > Texture (enhancement to existing Data Traffic)
- **Pipeline Position**: Same as current Data Traffic — generator stage
- **Scope**: Shader-only changes to `shaders/data_traffic.fs` plus new config fields and UI sliders

## References

- User-provided Shadertoy shader "Data Traffic" — full source with all behaviors
- Existing implementation: `shaders/data_traffic.fs`, `src/effects/data_traffic.h`

## Compatibility

All behaviors are structurally compatible. They modify the same per-cell values (`center`, `hw`, brightness, color) already computed in our shader's cell loop. No new textures, render passes, or compute shaders needed. Each is an additive code block gated by `if (hash < probability)`.

## Reference Code

The complete Shadertoy shader was provided in the brainstorm session. Below, each behavior section includes the verbatim code block extracted from that shader.

---

## Behavior Catalog

### Tier 1 — Trivial (1-5 lines)

#### 1. Row Flash

Random full-brightness flash on an entire row. Creates sudden lightning-like accents.

**Modifies**: brightness (row-level)
**Lines**: ~2

```glsl
// Row flash — at mainImage level, after cells
float rfl = h21(vec2(ri, floor(T * 1.5)));
if (rfl > .994) col = mix(col, vec3(1) * fog, .3);
```

**Params**: `flashRate` (flashes per second), `flashIntensity` (brightness boost)

---

#### 2. Breathing

Row gap size oscillates sinusoidally, making lanes pulse wider/narrower like breathing.

**Modifies**: `gapZ` (row gap)
**Lines**: ~4

```glsl
// Breathing — in getRowState
float breathSeed = h11(ri * .531 + 99.);
if (breathSeed < .3) {
    float breathRate = mix(.15, .4, h11(ri * .831 + 111.));
    float breath = sin(T * breathRate * 6.28) * .5 + .5;
    r.gapZ = mix(.02, .35, breath);
}
```

**Params**: `breathProb` (fraction of lanes), `breathRate` (oscillation speed)

---

### Tier 2 — Simple (6-10 lines)

#### 3. Heartbeat

Dual-Gaussian brightness pulse (lub-dub) with red color tint. Cells throb like a heartbeat.

**Modifies**: brightness, color
**Lines**: ~8

```glsl
// Heartbeat — in getCellInfo
float heartSeed = h21(vec2(ci * 11.1, ri + 678.));
if (heartSeed < .1) {
    float heartRate = mix(.8, 1.4, h21(vec2(ci + 1500., ri)));
    float heartPhase = fract(T * heartRate + heartSeed * 10.);
    float lub = exp(-pow((heartPhase - .1) * 15., 2.));
    float dub = exp(-pow((heartPhase - .25) * 18., 2.)) * .7;
    float heartBeat = max(lub, dub);
    i.brightness = max(i.brightness, heartBeat);
    if (heartBeat > .3) i.color = mix(i.color, vec3(.9, .1, .15), heartBeat * .6);
}
```

**Params**: `heartbeatProb` (fraction of cells), `heartbeatRate` (bpm-like speed)

---

#### 4. Per-Cell Twitch

Multi-frequency sinusoidal position oscillation with burst envelope. Cells jitter nervously in short bursts.

**Modifies**: `center` (position)
**Lines**: ~8

```glsl
// Per-cell twitch — in getCellInfo
float twitchSeed = h21(vec2(ci * 7.3, ri * 3.1 + 55.));
if (twitchSeed < .15) {
    float twitchRate = mix(8., 25., h21(vec2(ci + 500., ri)));
    float twitchAmp = sp * mix(.08, .25, h21(vec2(ci + 600., ri)));
    float twitch = sin(T * twitchRate + ci * 3.7) * .6
                 + sin(T * twitchRate * 1.7 + ci * 5.1) * .3
                 + sin(T * twitchRate * 3.1 + ci * 9.3) * .1;
    float burstSeed = h21(vec2(ci + 700., ri));
    float burst = smoothstep(.3, .5, sin(T * mix(.3, .8, burstSeed) + burstSeed * 6.28));
    i.center += twitch * twitchAmp * burst;
}
```

**Params**: `twitchProb` (fraction of cells), `twitchIntensity` (amplitude scale)

---

#### 5. Split / Merge

Cells momentarily shrink to nearly zero width (split) or expand wide (merge). Visual blinking and pulsing.

**Modifies**: `hw` (width)
**Lines**: ~8

```glsl
// Split — in getCellInfo (uses existing width epoch vars wE, wER, s3)
float spS = h21(vec2(ci + wE * 11.3, ri + 77.));
if (spS < .25) {
    float p = fract(T * wER + s3 * 12.);
    float c = smoothstep(0., .1, p) * (1. - smoothstep(.5, .8, p));
    i.hw *= mix(1., .05, c);
}
// Merge
float mS = h21(vec2(ci + wE * 5.7, ri + 33.));
if (mS < .15 && spS >= .25) {
    float p = fract(T * wER + s3 * 12.);
    float e = smoothstep(0., .2, p) * (1. - smoothstep(.6, .9, p));
    i.hw *= mix(1., 2.5, e);
}
```

**Params**: `splitProb`, `mergeProb`

---

#### 6. Phase Shift

Sudden speed kick at row level with damped spring wobble. Lanes jolt forward then settle.

**Modifies**: `speed` (row-level)
**Lines**: ~8

```glsl
// Phase shift — in getRowState
float phaseSeed = h11(ri * .371 + 222.);
if (phaseSeed < .15) {
    float phaseRate = mix(.08, .2, h11(ri * .621 + 333.));
    float phaseEpoch = floor(T * phaseRate + phaseSeed * 20.);
    float phaseT = fract(T * phaseRate + phaseSeed * 20.);
    float phaseDir = h11(ri * .441 + phaseEpoch * 7.7) > .5 ? 1. : -1.;
    if (phaseT < .15) {
        float snapT2 = phaseT / .15;
        r.speed += phaseDir * 8. * (1. - snapT2);
    } else if (phaseT < .6) {
        float wobT = (phaseT - .15) / .45;
        float kick2 = exp(-4. * wobT) * cos(wobT * 40.);
        r.speed += phaseDir * 3. * kick2;
    }
}
```

**Params**: `phaseShiftProb` (fraction of lanes), `phaseShiftIntensity`

---

#### 7. Doorstop Spring (Position)

Cell gets kicked to a random position then wobbles back with damped oscillation. Like flicking a doorstop.

**Modifies**: `center` (position)
**Lines**: ~8

```glsl
// Doorstop spring position — in getCellInfo
float springChance = h21(vec2(ci * 4.1, ri + 123.));
if (springChance < .35) {
    float springRate = mix(.15, .5, h21(vec2(ci + 800., ri)));
    float springEpoch = floor(T * springRate + s2 * 10.);
    float springT = fract(T * springRate + s2 * 10.);
    float target = (h21(vec2(ci + springEpoch * 5.1, ri + 90.)) - .5) * sp * 5.;
    float kick = exp(-3. * springT);
    float wobble = cos(springT * 50.);
    i.center += target * kick * wobble;
}
```

**Params**: `springProb`, `springIntensity`

---

#### 8. Doorstop Spring (Width)

Same damped oscillation but applied to cell width. Cells bounce between wide and narrow.

**Modifies**: `hw` (width)
**Lines**: ~8

```glsl
// Doorstop width spring — in getCellInfo
float springWSeed = h21(vec2(ci * 3.3, ri + 155.));
if (springWSeed < .25) {
    float swRate = mix(.15, .4, h21(vec2(ci + 900., ri)));
    float swEpoch = floor(T * swRate + s4 * 8.);
    float swT = fract(T * swRate + s4 * 8.);
    float wKick = mix(.3, 3., h21(vec2(ci + swEpoch * 7.7, ri + 111.)));
    float wDecay = exp(-3.5 * swT);
    float wWobble = cos(swT * 40.);
    float wMul = 1. + (wKick - 1.) * wDecay * wWobble;
    i.hw *= max(wMul, .02);
}
```

**Params**: `widthSpringProb`, `widthSpringIntensity`

---

### Tier 3 — Medium (10-15 lines)

#### 9. Stutter / Glitch

Cell freezes in place (scroll offset stuck), then snaps back with a spring kick. Creates a glitchy lag effect.

**Modifies**: `center` (position)
**Lines**: ~10

```glsl
// Stutter/glitch — in getCellInfo
float glitchSeed = h21(vec2(ci * 5.5, ri + 234.));
if (glitchSeed < .1) {
    float glitchRate = mix(.2, .6, h21(vec2(ci + 1100., ri)));
    float glitchEpoch = floor(T * glitchRate + glitchSeed * 20.);
    float glitchT = fract(T * glitchRate + glitchSeed * 20.);
    if (glitchT < .4) {
        float freezeOffset = -T * .3 * glitchT;
        i.center += freezeOffset * h21(vec2(ci + glitchEpoch * 3.3, ri));
    } else if (glitchT < .5) {
        float snapT = (glitchT - .4) / .1;
        float snapKick = exp(-5. * snapT) * cos(snapT * 35.);
        i.center += snapKick * sp * .8;
    }
}
```

**Params**: `stutterProb`, `stutterIntensity`

---

#### 10. Fission

Cell splits into two narrower copies that drift apart, hold, then recombine. Like cell division.

**Modifies**: `center` (position), `hw` (width)
**Lines**: ~12

```glsl
// Fission — in getCellInfo
float fissionSeed = h21(vec2(ci * 6.6, ri + 345.));
if (fissionSeed < .08) {
    float fissionRate = mix(.1, .3, h21(vec2(ci + 1200., ri)));
    float fissionEpoch = floor(T * fissionRate + fissionSeed * 15.);
    float fissionT = fract(T * fissionRate + fissionSeed * 15.);
    float fissionActive = h21(vec2(ci + fissionEpoch * 9.9, ri + 456.));
    if (fissionActive < .5) {
        float splitPhase;
        if (fissionT < .2) splitPhase = smoothstep(0., .2, fissionT);
        else if (fissionT < .7) splitPhase = 1.;
        else splitPhase = 1. - smoothstep(.7, .9, fissionT);
        i.hw *= mix(1., .4, splitPhase);
        float drift = splitPhase * sp * .4;
        float halfSide = h21(vec2(ci * 1.1, ri + fissionEpoch * 2.2)) > .5 ? 1. : -1.;
        i.center += drift * halfSide;
    }
}
```

**Params**: `fissionProb`, `fissionIntensity`

---

#### 11. Magnetic Snap

Cell accelerates toward a neighbor, sticks briefly, then gets repelled back with a spring. Attract-hold-repel cycle.

**Modifies**: `center` (position), `hw` (width)
**Lines**: ~12

```glsl
// Magnetic snap — in getCellInfo
float magSeed = h21(vec2(ci * 8.8, ri + 456.));
if (magSeed < .12) {
    float magRate = mix(.15, .4, h21(vec2(ci + 1300., ri)));
    float magEpoch = floor(T * magRate + magSeed * 12.);
    float magT = fract(T * magRate + magSeed * 12.);
    float magDir = h21(vec2(ci + magEpoch * 6.6, ri + 567.)) > .5 ? 1. : -1.;
    if (magT < .3) {
        float at = magT / .3;
        i.center += at * at * sp * .5 * magDir;
        i.hw *= mix(1., 1.4, at);
    } else if (magT < .5) {
        i.center += sp * .5 * magDir;
        i.hw *= 1.4;
    } else if (magT < .8) {
        float rt = (magT - .5) / .3;
        float repelKick = exp(-4. * rt) * cos(rt * 30.);
        i.center += sp * .5 * magDir * repelKick;
        i.hw *= mix(1.4, 1., rt);
    }
}
```

**Params**: `magnetProb`, `magnetIntensity`

---

#### 12. Ricochet

Cell launches in one direction, bounces off an invisible wall, and returns. Like a ball thrown against a surface.

**Modifies**: `center` (position)
**Lines**: ~15

```glsl
// Ricochet — in getCellInfo
float ricoSeed = h21(vec2(ci * 9.9, ri + 567.));
if (ricoSeed < .08) {
    float ricoRate = mix(.1, .3, h21(vec2(ci + 1400., ri)));
    float ricoEpoch = floor(T * ricoRate + ricoSeed * 18.);
    float ricoT = fract(T * ricoRate + ricoSeed * 18.);
    float ricoDir = h21(vec2(ci + ricoEpoch * 4.4, ri + 678.)) > .5 ? 1. : -1.;
    float ricoActive = h21(vec2(ci + ricoEpoch * 7.1, ri + 789.));
    if (ricoActive < .4) {
        float maxDist = sp * 2.;
        if (ricoT < .25) {
            float ot = ricoT / .25;
            i.center += ot * maxDist * ricoDir;
        } else if (ricoT < .35) {
            float bt = (ricoT - .25) / .1;
            float bounce = (1. - bt * .5) * cos(bt * 20.);
            i.center += maxDist * ricoDir * bounce;
        } else if (ricoT < .7) {
            float rt = (ricoT - .35) / .35;
            float returnDist = (1. - rt * rt) * maxDist * .5;
            i.center += returnDist * ricoDir;
        }
    }
}
```

**Params**: `ricochetProb`, `ricochetIntensity`

---

#### 13. Proximity Glow

Gaussian soft light bleed from bright/colored cells across lane boundaries. Cells illuminate their surroundings.

**Modifies**: color (additive, across rows)
**Lines**: ~15
**Note**: This is the only behavior that requires a **separate loop** outside the cell loop — it iterates over neighboring rows and nearby cells to accumulate glow. Structurally different from the others.

```glsl
// Proximity glow — in mainImage, before cell rendering
vec3 gw = vec3(0);
if (t < 80.) {
    for (int ro2 = -1; ro2 <= 1; ro2++) {
        float gri = ri + float(ro2);
        RowState gs = getRowState(gri);
        float gx = wp.x + T * gs.speed * .4;
        float gci = floor(gx / gs.spacing);
        for (int di = -3; di <= 3; di++) {
            float ci = gci + float(di);
            GlowCell gc = getGlowCell(ci, gri, gs.baseW, gs.rh3, gs.rh5, gs.rh6, gs.spacing);
            if (!gc.emit) continue;
            float dx = abs(gx - gc.center);
            float dz = abs(float(ro2)) / rowScale;
            float d2 = dx * dx + dz * dz * 12.;
            float gr = gs.baseW * 5.;
            gw += gc.color * exp(-d2 / (gr * gr * .15)) * .45;
        }
    }
}
col += gw * fog;
```

**Params**: `glowIntensity` (already reserved at 0.0 in current config), `glowRadius`

---

### Tier 4 — Complex (15-25+ lines)

#### 14. Wave Propagation + Color Overlay

Traveling displacement waves sweep along a row, pushing cells as they pass. Paired with a color tint that rides the wave. Up to 3 concurrent waves per row.

**Modifies**: `center` (position via wave propagation), color (via wave color overlay)
**Lines**: ~25 total (propagation + color overlay)

```glsl
// Wave propagation — in getCellInfo (position displacement)
for (int w = 0; w < 3; w++) {
    float wS = h21(vec2(ri * 3. + float(w), 91.7));
    float wS2 = h21(vec2(ri * 3. + float(w), 173.3));
    float wS3 = h21(vec2(ri * 3. + float(w), 257.1));
    float wP = mix(3., 8., wS), wSp = mix(4., 15., wS2), wD = wS3 > .5 ? 1. : -1.;
    float wE = floor(T / wP + wS * 20.);
    float wAc = h21(vec2(ri + wE * 7.7, float(w) + 50.));
    if (wAc < .5) {
        float wT = fract(T / wP + wS * 20.) * wP;
        float d = ci / wSp;
        float lT = wT - d * wD;
        if (lT > 0. && lT < 1.2) {
            float b = exp(-lT * 5.) * sin(lT * 12.);
            j += sp * mix(.3, .7, wS) * b * wD;
        }
    }
}

// Wave color overlay — in getCellInfo (color tinting)
float wI = 0.; vec3 wC = vec3(0);
for (int w = 0; w < 3; w++) {
    float wS = h21(vec2(ri * 3. + float(w), 91.7));
    float wS2 = h21(vec2(ri * 3. + float(w), 173.3));
    float wS3 = h21(vec2(ri * 3. + float(w), 257.1));
    float wP = mix(3., 8., wS), wSp = mix(4., 15., wS2), wD = wS3 > .5 ? 1. : -1.;
    float wE2 = floor(T / wP + wS * 20.);
    float wAc = h21(vec2(ri + wE2 * 7.7, float(w) + 50.));
    if (wAc < .5) {
        float wT = fract(T / wP + wS * 20.) * wP;
        float d = ci / wSp;
        float lT = wT - d * wD;
        if (lT > -.2 && lT < 1.5) {
            float en = smoothstep(-.2, 0., lT) * exp(-max(lT, 0.) * 3.);
            wI = max(wI, en);
        }
        float wH = h21(vec2(ri + wE2 * 3.3, float(w) + 200.));
        vec3 wc;
        if (wH < .17) wc = vec3(.9, .15, .1);
        else if (wH < .33) wc = vec3(.1, .8, .9);
        else if (wH < .5) wc = vec3(.95, .6, .05);
        else if (wH < .67) wc = vec3(.7, .1, .9);
        else if (wH < .83) wc = vec3(.1, .9, .3);
        else wc = vec3(.3, .4, 1.);
        wC = wc;
    }
}
if (wI > .05) {
    float wb = max(i.brightness, .5) * (.8 + .4 * wI);
    i.color = mix(i.color, wC * wb, wI);
    i.isColor = true;
}
```

**Adaptation**: Replace hardcoded 6-color palette in wave overlay with `gradientLUT` sampling.

**Params**: `waveIntensity` (master scale for both displacement and color), `waveCount` (1-3)

---

#### 15. Domino Cascade

Sequential chain reaction along a row — cells lean over one by one in a wave, then spring back. Direction and timing are hash-randomized per epoch.

**Modifies**: `center` (position), `hw` (width — cells compress as they lean)
**Lines**: ~15

```glsl
// Domino cascade — in getCellInfo
float dominoSeed = h21(vec2(ri * 2.1, 567.));
float dominoPeriod = mix(4., 8., dominoSeed);
float dominoEpoch = floor(T / dominoPeriod + dominoSeed * 15.);
float dominoActive = h21(vec2(ri + dominoEpoch * 4.4, 678.));
if (dominoActive < .25) {
    float dominoT = fract(T / dominoPeriod + dominoSeed * 15.) * dominoPeriod;
    float dominoSpeed = mix(6., 12., h21(vec2(ri + dominoEpoch * 2.2, 789.)));
    float dominoDir = h21(vec2(ri + dominoEpoch * 8.8, 890.)) > .5 ? 1. : -1.;
    float cellDelay = ci / dominoSpeed * dominoDir;
    float localDT = dominoT - cellDelay;
    if (localDT > 0. && localDT < .8) {
        float lean = sin(localDT * 5.) * exp(-localDT * 4.);
        i.center += lean * sp * .6 * dominoDir;
        i.hw *= mix(1., .5, abs(lean));
    }
}
```

**Params**: `dominoProb` (fraction of lanes affected), `dominoIntensity`

---

#### 16. Newton's Cradle

Momentum transfer across a cluster of adjacent cells. An end cell swings in, the interior cells bump in sequence, and the opposite end cell swings out. The most complex behavior — ~25 lines with multiple phases.

**Modifies**: `center` (position)
**Lines**: ~25

```glsl
// Newton's cradle — in getCellInfo
float cradleSeed = h21(vec2(ri, 77.7));
float cradlePeriod = mix(2.5, 5., cradleSeed);
float cradleEpoch = floor(T / cradlePeriod + cradleSeed * 10.);
float cradleActive = h21(vec2(ri + cradleEpoch * 3.3, 199.));
if (cradleActive < .4) {
    float cradleT = fract(T / cradlePeriod + cradleSeed * 10.) * cradlePeriod;
    float clusterCenter = floor(h21(vec2(ri + cradleEpoch * 11.1, 333.)) * 20. - 10.);
    float clusterSize = 5.; float halfCluster = clusterSize * .5;
    float posInCluster = ci - clusterCenter;
    if (abs(posInCluster) <= halfCluster + 1.5) {
        float cDir = h21(vec2(ri + cradleEpoch * 5.5, 444.)) > .5 ? 1. : -1.;
        float swingDur = .6; float propDelay = .04;
        float inPos = -cDir * (halfCluster + 1.);
        float outPos = cDir * (halfCluster + 1.);
        float distFromIn = abs(posInCluster - inPos);
        float distFromOut = abs(posInCluster - outPos);
        if (distFromIn < 1.5) {
            float phase = cradleT / swingDur; float swing;
            if (phase < 1.) swing = -sin(phase * 3.14159) * sp * 1.8 * cDir;
            else if (phase < 1.3) swing = 0.;
            else if (phase < 2.3) {
                float p2 = (phase - 1.3) / 1.;
                swing = -sin(p2 * 3.14159 * .5) * sp * .8 * cDir;
            } else swing = 0.;
            j += swing;
        } else if (distFromOut < 1.5) {
            float delay = clusterSize * propDelay;
            float phase = (cradleT - swingDur - delay) / swingDur;
            if (phase > 0. && phase < 1.) {
                float swing = sin(phase * 3.14159) * sp * 1.8 * cDir;
                j += swing;
            } else if (phase >= 1. && phase < 2.) {
                float p2 = phase - 1.;
                float swing = sin(p2 * 3.14159 * .5) * sp * .5 * cDir;
                j += swing;
            }
        } else if (abs(posInCluster) <= halfCluster) {
            float cellDelay = (posInCluster * cDir + halfCluster) * propDelay;
            float localT = cradleT - swingDur * .9 - cellDelay;
            if (localT > 0. && localT < .15) {
                float bump = sin(localT / .15 * 3.14159) * sp * .15 * cDir;
                j += bump;
            }
        }
    }
}
```

**Params**: `cradleProb` (fraction of lanes), `cradleIntensity`

---

#### 17. Width Easing Variations

Multiple easing curve types for width transitions — smoothstep, elastic, bounce, and spring. Each cell is randomly assigned one type. This replaces the existing simple `smoothstep` width interpolation with richer motion.

**Modifies**: `hw` (width transition curve)
**Lines**: ~25 (mostly branching)

```glsl
// Width easing — in getCellInfo (replaces simple width interpolation)
float easingType = floor(h21(vec2(ci * 2.7, ri + 88.)) * 4.);
float wER = mix(.15, .5, rh6);
float wE = floor(T * wER + s3 * 12.), wEN = wE + 1.;
float wB = fract(T * wER + s3 * 12.);
float w0 = h21(vec2(ci + wE * 3.7, ri + 50.));
float w1 = h21(vec2(ci + wEN * 3.7, ri + 50.));
float eB;
if (easingType < 1.)
    eB = smoothstep(0., .3, wB) * (1. - smoothstep(.7, 1., wB)) + smoothstep(.7, 1., wB);
else if (easingType < 2.) {
    float t2 = clamp(wB * 1.2, 0., 1.);
    eB = t2 < .01 ? 0. : 1. - pow(2., -10. * t2) * sin((t2 * 10. - .75) * 2.094);
    eB = clamp(eB, 0., 1.);
} else if (easingType < 3.) {
    float t2 = clamp(wB * 1.1, 0., 1.); float n1 = 7.5625, d1 = 2.75;
    if (t2 < 1. / d1) eB = n1 * t2 * t2;
    else if (t2 < 2. / d1) { t2 -= 1.5 / d1; eB = n1 * t2 * t2 + .75; }
    else if (t2 < 2.5 / d1) { t2 -= 2.25 / d1; eB = n1 * t2 * t2 + .9375; }
    else { t2 -= 2.625 / d1; eB = n1 * t2 * t2 + .984375; }
} else {
    float t2 = clamp(wB * 1.3, 0., 1.);
    float decay = exp(-5. * t2); float osc = cos(t2 * 18.);
    eB = 1. - decay * osc; eB = clamp(eB, 0., 1.);
}
float ws = mix(w0, w1, eB);
i.hw = bW * mix(.05, 3., ws * rh3 + (1. - rh3) * ws * ws);
```

**Params**: `easingVariety` (0.0 = all smoothstep, 1.0 = full random easing type mix)

---

#### 18. Row Speed Easing

Multiple easing types for row scroll speed transitions — smoothstep with ramp, doorstop spring with stop, and overshoot spring. Replaces the current simple smoothstep speed interpolation.

**Modifies**: `speed` (row-level transition curve)
**Lines**: ~20

```glsl
// Row speed easing — in getRowState (replaces simple speed ramp)
float easeType = h11(ri * .777 + ep * .333);
float rmp;
if (easeType < .4)
    rmp = smoothstep(0., .12, et) * smoothstep(1., .75, et);
else if (easeType < .7) {
    float accel = smoothstep(0., .05, et); float stopAt = .4;
    if (et > stopAt) {
        float st = (et - stopAt) / (1. - stopAt);
        float kick2 = exp(-3.5 * st); float wobble2 = sin(st * 55.);
        rmp = accel * kick2 * wobble2;
    } else rmp = accel;
} else {
    float accel = smoothstep(0., .03, et); float stopAt = .35;
    if (et > stopAt) {
        float st = (et - stopAt) / (1. - stopAt);
        float kick2 = exp(-2.5 * st); float wobble2 = sin(st * 40. + 1.57);
        rmp = accel * kick2 * wobble2 * 1.3;
    } else rmp = accel;
}
r.speed = bs * db;
if (es < .2) r.speed = 0.;
r.speed *= rmp;
```

**Params**: `speedEasingVariety` (0.0 = smooth only, 1.0 = spring/overshoot mix)

---

## Potential Additions (Not in Reference)

### A. Audio-Gated Behaviors

Instead of pure hash probability, gate behavior activation on FFT energy. Dominos trigger only during loud sections. Heartbeats sync to bass. Fission fires on transients. This turns behaviors into song-reactive events rather than random occurrences.

**Params**: `audioGating` (0.0 = pure hash, 1.0 = fully audio-gated)

### B. Scroll Direction Reversal

Individual lanes occasionally reverse scroll direction with spring easing. Distinct from phase shift (which is a temporary speed kick) — this is a sustained reversal that persists for an epoch.

**Params**: `reversalProb`, `reversalEasing`

### C. Cell Subdivision

A cell temporarily divides into 2-4 smaller sub-cells within its original footprint, each with slightly different brightness. Like a detail zoom. Different from fission (which drifts the halves apart) — subdivision keeps cells packed within the original bounds.

**Params**: `subdivProb`, `subdivCount` (2-4)

### D. Inter-Lane Sparks

Vertical sparks between aligned cells in adjacent lanes. Currently sparks only fire horizontally between cells in the same lane. Vertical sparks would create column-like flashes connecting the grid vertically.

**Params**: `verticalSparkIntensity`

### E. Packet Burst

Groups of 3-8 consecutive cells briefly flash in sequence with a propagation delay, simulating data packet transmission. Like a mini domino but brightness-only (no position displacement).

**Params**: `packetProb`, `packetLength` (3-8), `packetSpeed`

---

## Implementation Notes

- All behaviors stack additively on `center` and multiplicatively on `hw` — order within the cell loop doesn't matter
- The reference uses separate probability thresholds per behavior (e.g., `< .35`, `< .08`, `< .1`). Our params would replace these with uniform values
- Width easing and row speed easing are replacements for existing interpolation, not additive behaviors — implementing them means swapping the current `smoothstep` calls
- Proximity glow is architecturally different: it needs a separate loop over neighboring rows BEFORE the main cell loop. All other behaviors fit inside the existing cell loop
- Shader instruction count grows linearly with enabled behaviors. At full complexity (all 18 enabled), expect ~2-3x the current shader cost. Each behavior's probability gate (`if (hash < prob)`) means disabled behaviors are essentially free (branch not taken)

## Modulation Candidates

All probability params (`*Prob`) and intensity params (`*Intensity`) are modulatable. Key interactions:

### Interaction Patterns

**Cascading threshold (springProb + stutterProb):** When both are high, cells that spring also stutter — the stutter freezes mid-spring creating a glitch-in-a-glitch effect. At low values they're independent rarities.

**Competing forces (phaseShiftIntensity + scrollSpeed):** Phase shift kicks against the base scroll. High phase shift with slow scroll creates lurching stop-start motion. High phase shift with fast scroll creates barely-noticeable hiccups. The ratio determines the character.

**Resonance (dominoProb + waveIntensity):** When a wave propagation coincides with a domino cascade in the same row, cells get double-displaced creating dramatic chain reactions. Rare at low values, visually explosive when both peak together during a chorus.
