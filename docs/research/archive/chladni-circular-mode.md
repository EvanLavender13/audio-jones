# Chladni Circular Plate Mode

Adding circular (drum membrane) plate modes to the existing Chladni generator. A `plateShape` parameter switches between the current rectangular eigenmode equation and circular Bessel function modes. A `fullscreen` toggle controls whether the pattern is masked to the plate boundary or fills the screen, applicable to both shapes.

## Classification

- **Category**: Enhancement to existing GENERATORS > Cymatics > Chladni
- **Pipeline Position**: Same generator stage, same ping-pong trail infrastructure
- **Compute Model**: Fragment shader modification (no new passes or textures)

## References

- [Paul Bourke - Chladni Plate Interference Surfaces](https://paulbourke.net/geometry/chladni/) - Rectangular and circular plate equations, mode shape gallery
- [Vibration of a circular membrane - Wikipedia](https://en.wikipedia.org/wiki/Vibration_of_a_circular_membrane) - Bessel function mode shapes, frequency relationships
- [Bessel Function Zeros - Wolfram MathWorld](https://mathworld.wolfram.com/BesselFunctionZeros.html) - Zeros table for J0-J5
- [Cephes j0.c](https://github.com/google-deepmind/torch-cephes/blob/master/cephes/bessel/j0.c) - Production rational polynomial approximation for J0
- [Cephes j1.c](https://github.com/google-deepmind/torch-cephes/blob/master/cephes/bessel/j1.c) - Production rational polynomial approximation for J1

## Reference Code

### Existing rectangular plate equation (from shaders/chladni.fs)

```glsl
float chladni(vec2 uv, float n_val, float m_val, float L) {
    float nx = n_val * PI / L;
    float mx = m_val * PI / L;
    return cos(nx * uv.x) * cos(mx * uv.y) - cos(mx * uv.x) * cos(nx * uv.y);
}
```

### Cephes J0 rational polynomial approximation

Domain [0, 5]: rational approximation `(z - DR1)(z - DR2) * P(z)/Q(z)` where z = x*x.
Domain (5, inf): Hankel asymptotic `sqrt(2/pi/x) * (P(q)*cos(x-pi/4) - (5/x)*Q(q)*sin(x-pi/4))` where q = 25/(x*x).

```c
// DR1, DR2 are squares of the first two zeros of J0
// DR1 = 5.78318..., DR2 = 30.4713...  (i.e. 2.4048^2 and 5.5201^2)

static double RP[4] = {
    -4.79443220978201773821E9,
     1.95617491946556577543E12,
    -2.49248344360967716204E14,
     9.70862251047306323952E15,
};

static double RQ[8] = {
     4.99563147152651017219E2,
     1.73785401676374683123E5,
     4.84409658339962045305E7,
     1.11855537045356834862E10,
     2.11277520115489217587E12,
     3.10518229857422583814E14,
     3.18121955943204943306E16,
     1.71086294081043136091E18,
};

static double PP[7] = {
     7.96936729297347051624E-4,
     8.28352392107440799803E-2,
     1.23953371646414299388E0,
     5.44725003058768775090E0,
     8.74716500199817011941E0,
     5.30324038235394892183E0,
     9.99999999999999997821E-1,
};

static double PQ[7] = {
     9.24408810558863637013E-4,
     8.56288474354474431428E-2,
     1.25352743901058953537E0,
     5.47097740330417105182E0,
     8.76190883237069594232E0,
     5.30605288235394617618E0,
     1.00000000000000000218E0,
};

static double QP[8] = {
    -1.13663838898469149931E-2,
    -1.28252718670509318512E0,
    -1.95539544257735972385E1,
    -9.32060152123768231369E1,
    -1.77681167980488050595E2,
    -1.47077505154951170175E2,
    -5.14105326766599330220E1,
    -6.05014350600728481186E0,
};

static double QQ[7] = {
     6.43178256118178023184E1,
     8.56430025976980587198E2,
     3.88240183605401609683E3,
     7.24046774195652478189E3,
     5.93072701187316984827E3,
     2.06209331660327847417E3,
     2.42005740240291393179E2,
};

double j0(double x) {
    if (x < 0) x = -x;
    if (x <= 5.0) {
        double z = x * x;
        if (x < 1.0e-5) return 1.0 - z / 4.0;
        double p = (z - DR1) * (z - DR2);
        p = p * polevl(z, RP, 3) / p1evl(z, RQ, 8);
        return p;
    }
    double w = 5.0 / x;
    double q = 25.0 / (x * x);
    double p = polevl(q, PP, 6) / polevl(q, PQ, 6);
    q = polevl(q, QP, 7) / p1evl(q, QQ, 7);
    double xn = x - PIO4;
    p = p * cos(xn) - w * q * sin(xn);
    return p * SQ2OPI / sqrt(x);
}
```

### Bessel function zeros table (for circular membrane modes)

The m-th zero of J_n, denoted alpha_{n,m}, determines the (n,m) circular plate mode:

| n\m | 1       | 2       | 3       | 4       | 5       |
|-----|---------|---------|---------|---------|---------|
| 0   | 2.4048  | 5.5201  | 8.6537  | 11.7915 | 14.9309 |
| 1   | 3.8317  | 7.0156  | 10.1735 | 13.3237 | 16.4706 |
| 2   | 5.1356  | 8.4172  | 11.6198 | 14.7960 | 17.9598 |
| 3   | 6.3802  | 9.7610  | 13.0152 | 16.2235 | 19.4094 |
| 4   | 7.5883  | 11.0647 | 14.3725 | 17.6160 | 20.8269 |
| 5   | 8.7715  | 12.3386 | 15.7002 | 18.9801 | 22.2178 |

## Algorithm

### Circular plate mode equation

The vibration pattern of a circular membrane clamped at radius R is:

```
Phi_{n,m}(r, theta) = J_n(alpha_{n,m} * r / R) * cos(n * theta)
```

Where:
- `J_n` is the Bessel function of the first kind, order n
- `alpha_{n,m}` is the m-th zero of `J_n` (from the table above)
- `r = length(uv)`, `theta = atan2(uv.y, uv.x)` in centered coordinates
- The pattern is zero at `r = R` (clamped boundary)

### Resonant frequency mapping

For a circular plate, the resonant frequency of mode (n,m) is proportional to `alpha_{n,m}^2` (same square relationship as rectangular modes where freq ~ n^2 + m^2). Map the lowest mode (0,1) with `alpha_{0,1} = 2.4048` to `baseFreq`:

```
freqScale = baseFreq / (2.4048^2)    // = baseFreq / 5.783
modeFreq(n,m) = freqScale * alpha_{n,m}^2
```

### Bessel function in GLSL

The Cephes rational polynomial approximation ports directly to GLSL. For the circular plate, only `J_0` through `J_5` are needed (orders 0-5 give 30 modes from the zeros table). Higher-order Bessel functions can use the recurrence relation:

```glsl
// Recurrence: J_{n+1}(x) = (2n/x) * J_n(x) - J_{n-1}(x)
float besselJ(int n, float x) {
    if (n == 0) return besselJ0(x);
    if (n == 1) return besselJ1(x);
    float jPrev = besselJ0(x);
    float jCurr = besselJ1(x);
    for (int i = 1; i < n; i++) {
        float jNext = (2.0 * float(i) / x) * jCurr - jPrev;
        jPrev = jCurr;
        jCurr = jNext;
    }
    return jCurr;
}
```

Only `besselJ0` and `besselJ1` need the full rational polynomial implementation; all higher orders derive from them via recurrence.

### GLSL J0 implementation (simplified from Cephes for mediump/highp float)

The double-precision Cephes code uses 4+8+7+7+8+7 = 41 coefficients. For GLSL float (23-bit mantissa), the polynomial degrees can be reduced. However, since the existing project targets desktop GL 3.30 with highp float, the full Cephes coefficients converted to float should work. The key structure:

```glsl
float besselJ0(float x) {
    x = abs(x);
    if (x <= 5.0) {
        float z = x * x;
        if (x < 1.0e-5) return 1.0 - z * 0.25;
        float p = (z - 5.7831859629) * (z - 30.4712623437);
        // Evaluate RP/RQ rational polynomial in z
        p *= polevl4(z, RP) / p1evl8(z, RQ);
        return p;
    }
    float w = 5.0 / x;
    float q = w * w;
    float p = polevl7(q, PP) / polevl7(q, PQ);
    float qq = polevl8(q, QP) / p1evl7(q, QQ);
    float xn = x - PI * 0.25;
    return (p * cos(xn) - w * qq * sin(xn)) * 0.797884560802865 / sqrt(x);
}
```

### What changes in the shader

- Add `uniform int plateShape;` (0=rectangular, 1=circular)
- Add `uniform int fullscreen;` (0=plate boundary, 1=fill screen)
- Store the 30 Bessel zeros as a `const float` array (6 orders x 5 zeros)
- In the mode loop: when `plateShape == 1`, use `besselJ(n, alpha_{n,m} * r / plateSize) * cos(n * theta)` instead of the rectangular `cos×cos - cos×cos` equation
- Fullscreen toggle: when 0, multiply output by `smoothstep(plateSize + 0.02, plateSize - 0.02, r)` for circular or a rect boundary for rectangular. When 1, no masking.

### What changes in the C++ side

- Add `int plateShape` and `bool fullscreen` to `ChladniConfig`
- Add uniform locations and binding in `ChladniEffectSetup`
- Add UI controls: Combo for plate shape, checkbox for fullscreen
- Register new params with modulation engine

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| plateShape | int | 0-1 | 0 | 0=rectangular, 1=circular |
| fullscreen | bool | - | true | true=fill screen, false=mask to plate boundary |

All existing Chladni parameters (plateSize, coherence, visualGain, nodalEmphasis, baseFreq, maxFreq, gain, curve, decay, diffusion) apply to both modes unchanged.

## Modulation Candidates

- **plateShape**: Not modulatable (discrete switch)
- **fullscreen**: Not modulatable (discrete switch)

Existing modulation candidates from the Chladni research doc remain unchanged. The circular mode adds visual variety but no new continuous parameters.

## Notes

- The Bessel recurrence `J_{n+1} = (2n/x)*J_n - J_{n-1}` is numerically unstable for large n and small x, but with n <= 5 this is not a concern.
- The 30 hardcoded zeros (6 orders x 5 zeros) cover modes up to (5,5). With `baseFreq=55` and `freqScale = 55/5.783 = 9.51`, the highest mode frequency is `9.51 * 22.2178^2 = 4693 Hz`. For coverage up to `maxFreq=14000`, more zeros would be needed — either extend the table or compute zeros via the asymptotic formula `alpha_{n,m} ~ (m + n/2 - 1/4) * PI` for large m.
- The asymptotic zero formula `alpha_{n,m} ~ PI * (m + n/2 - 1/4)` is accurate to < 1% for m >= 3 and can extend coverage indefinitely without a larger table.
- Performance: `besselJ0` and `besselJ1` are ~20 ALU ops each. The recurrence adds ~4 ops per order. Total cost per mode evaluation is comparable to the rectangular equation's 4 `cos` calls.
