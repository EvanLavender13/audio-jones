# Triangle-Wave fBM Domain Warp

Radial domain warp using nimitz's triangle-wave fBM noise. Extracted from the filaments generator for potential reimplementation as a standalone warp effect.

## Origin

nimitz "Filaments" (shadertoy.com/view/4lcSWs). The noise was used to organically distort the lookup space before computing segment distances, giving the filaments a tangled, organic feel.

## Algorithm

Triangle-wave fBM — a fractal noise built from `abs(fract(x) - 0.5)` instead of sine or Perlin. Produces sharp creases rather than smooth hills.

```glsl
float tri(float x) { return abs(fract(x) - 0.5); }

vec2 tri2(vec2 p) {
    return vec2(tri(p.x + tri(p.y * 2.0)), tri(p.y + tri(p.x * 2.0)));
}

float triangleNoise(vec2 p, float time) {
    float z = 1.5;
    float z2 = 1.5;
    float rz = 0.0;
    vec2 bp = p * 0.8;
    for (int i = 0; i <= 3; i++) {
        vec2 dg = tri2(bp * 2.0) * 0.5;
        dg *= rot(time);            // animate crease directions
        p += dg / z2;               // warp coordinates by crease gradient
        bp *= 1.5;
        z2 *= 0.6;
        z *= 1.7;
        p *= 1.2;
        p *= mat2(0.970, 0.242, -0.242, 0.970);  // slight rotation per octave
        rz += tri(p.x + tri(p.y)) / z;
    }
    return rz;
}
```

## How It Was Applied

Multiplicative radial scaling centered at origin:

```glsl
float nz = clamp(triangleNoise(p), 0.0, 1.0);
p *= 1.0 + (nz - 0.5) * noiseStrength;
```

This pushes/pulls pixels radially based on the noise value. Equivalent to a post-process warp that displaces UV coordinates by the same radial factor.

## Parameters

| Name | Role | Range | Default |
|------|------|-------|---------|
| noiseStrength | Displacement magnitude | 0.0-1.0 | 0.4 |
| noiseSpeed | Animation rate (rad/s for crease rotation) | 0.0-10.0 | 4.5 |

## Notes for Reimplementation

- The `rot(time)` inside the noise loop rotates the crease directions, creating a slowly churning organic feel
- The per-octave coordinate rotation (`mat2(0.970, 0.242, ...)`) prevents axis-aligned artifacts
- The warp was radial (multiplicative on `p`), but could also be implemented as additive UV displacement for a different character
- As a standalone warp effect, the noise function and displacement mode (radial vs directional) would be the core parameters
