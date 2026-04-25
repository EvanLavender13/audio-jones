# Geode

A spherical cluster of randomly-rotated crystal cubes floating in black space, carved open by a 3D gyroid lattice or Perlin noise field. The outer shell reads as a mineral rind; the carved interior reveals a luminous cavern gradient. A slow camera orbit shows every face; independently, the cut field drifts and pulses so caverns open on one side while closing on another. Full lighting with self-shadowing, AO, specular highlights, and distance fog gives each cube crystal-like depth.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Generator stage (section 13)
- **Compute Model**: Single-pass shader (no feedback, no ping-pong)

## Attribution

- **Based on**: "randomly rotated cubes noise cut" by jt (Jakob Thomsen)
- **Source**: https://www.shadertoy.com/view/tXKXzh
- **License**: MIT
- **DDA renderer**: "dda voxel no div raymarch subobj" by jt, https://www.shadertoy.com/view/3XBSW1 (inlined inside the reference above)

## References

- "randomly rotated cubes noise cut" (Shadertoy tXKXzh) - Complete reference: DDA voxel march, gyroid/noise cut field, random-rotated cube SDF, shadow pass, AO, fog, rainbow palette.
- "Hash without Sine" by Dave_Hoskins (Shadertoy 4djSRW) - Hash functions used verbatim.
- "triple32 / lowbias32" by Chris Wellons / FabriceNeyret2 - Integer hash functions used verbatim.
- iquilezles.org `distfunctions` - box SDF, tetrahedron normal pattern.
- iquilezles.org `Xds3zN` - ambient occlusion loop.

## Reference Code

```glsl
// https://www.shadertoy.com/view/tXKXzh randomly rotated cubes noise cut, 2025 by jt
// using renderer: https://www.shadertoy.com/view/3XBSW1 dda voxel no div raymarch subobj

// Performance test:
// Cutting randomly rotated cubes material (pyrite?) by 3d noise (or gyroid).
// Unfortunately slow due to sampling with neighbors (i.e. factor 8 per voxel).

// Motivation:
// The usual axis-aligned, minecraft/lego voxel style feels to rigid.
// To relax the layout a bit, simply rotating the voxels to achieve a Pyrite style instead, feels like a minimal next step.
// Unfortunately even this little step appears to require sampling neighbors. Fortunately 2^3 is sufficient (I started with 3^3 first).

// tags: 3d, noise, grid, cube, voxel, random, rotate, dda, intersect, cut, pyrite

// The MIT License
// Copyright (c) 2025 Jakob Thomsen
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#define SPEEDUP_CHEAT

// Gyroid should be faster than noise
#define GYROID
// Noise is relatively heavy-weight
//#define NOISE

// used to avoid unrolling loops
#define ZERO (min(0,iFrame))

#define N 2
#define MAX_VOXEL_STEPS (64*N)
#define MAX_MARCH_STEPS 100
#define DIST_MAX (100.0*float(N))

#define pi 3.1415926
#define tau (pi+pi)

float hash12(vec2 p) // https://www.shadertoy.com/view/4djSRW Hash without Sine by Dave_Hoskins
{
    vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float hash13(vec3 p3) // https://www.shadertoy.com/view/4djSRW Hash without Sine by Dave_Hoskins
{
    p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 hash32(vec2 p) // https://www.shadertoy.com/view/4djSRW Hash without Sine by Dave_Hoskins
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

vec3 hash33(vec3 p3) // https://www.shadertoy.com/view/4djSRW Hash without Sine by Dave_Hoskins
{
    p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy + p3.yxx)*p3.zyx);
}

mat3 yaw_pitch_roll(float yaw, float pitch, float roll)
{
    mat3 R = mat3(vec3(cos(yaw), sin(yaw), 0.0), vec3(-sin(yaw), cos(yaw), 0.0), vec3(0.0, 0.0, 1.0));
    mat3 S = mat3(vec3(1.0, 0.0, 0.0), vec3(0.0, cos(pitch), sin(pitch)), vec3(0.0, -sin(pitch), cos(pitch)));
    mat3 T = mat3(vec3(cos(roll), 0.0, sin(roll)), vec3(0.0, 1.0, 0.0), vec3(-sin(roll), 0.0, cos(roll)));

    return R * S * T;
}

// https://www.shadertoy.com/view/WttXWX "Best" Integer Hash by FabriceNeyret2,
// implementing Chris Wellons https://nullprogram.com/blog/2018/07/31/
uint triple32(uint x)
{
    x ^= x >> 17;
    x *= 0xed5ad4bbU;
    x ^= x >> 11;
    x *= 0xac4c1b51U;
    x ^= x >> 15;
    x *= 0x31848babU;
    x ^= x >> 14;
    return x;
}

// Wellons lowbias32 from http://nullprogram.com/blog/2018/07/31
// perhaps the best ever devised for this op count, great distribution and cycle
uint lowbias32(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

#define HASH(u) triple32(u)
//#define HASH(u) lowbias32(u)

uint uhash(uvec2 v)
{
//return uvec2(0u); // verify grid alignment
    return HASH((uint(v.x) + HASH(uint(v.y))));
}

vec3 grad(ivec3 v)
{
    return hash33(vec3(v))*2.0-1.0;
}

float noise(vec3 p)
{
    ivec3 i = ivec3(floor(p));
    vec3  f = fract(p);

    vec3 u = smoothstep(vec3(0.0),vec3(1.0),f);
    return // 3d version of https://www.shadertoy.com/view/XdXGW8 Perlin noise by inigo quilez - iq/2013
        mix
        (
            mix
            (
                mix
                (
                    dot(grad(i+ivec3(0,0,0)), f-vec3(0,0,0)),
                    dot(grad(i+ivec3(1,0,0)), f-vec3(1,0,0)),
                    u.x
                ),
                mix
                (
                    dot(grad(i+ivec3(0,1,0)), f-vec3(0,1,0)),
                    dot(grad(i+ivec3(1,1,0)), f-vec3(1,1,0)),
                    u.x
                ),
                u.y
            ),
            mix
            (
                mix
                (
                    dot(grad(i+ivec3(0,0,1)), f-vec3(0,0,1)),
                    dot(grad(i+ivec3(1,0,1)), f-vec3(1,0,1)),
                    u.x
                ),
                mix
                (
                    dot(grad(i+ivec3(0,1,1)), f-vec3(0,1,1)),
                    dot(grad(i+ivec3(1,1,1)), f-vec3(1,1,1)),
                    u.x
                ),
                u.y
            ),
            u.z
        );
}

float gyroid(vec3 p)
{
    return dot(sin(p.xyz*tau),cos(p.yzx*tau));
}

float box(vec3 p, vec3 s) // adapted from https://iquilezles.org/articles/distfunctions/
{
    vec3 d = abs(p) - s;
    return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

vec4 random_unit_quaternion(vec3 r) // r in [0,1] https://stackoverflow.com/questions/31600717/how-to-generate-a-random-quaternion-quickly
{
    return
        vec4
        (
            sqrt(1.0-r.x) * sin(2.0*pi*r.y),
            sqrt(1.0-r.x) * cos(2.0*pi*r.y),
            sqrt(    r.x) * sin(2.0*pi*r.z),
            sqrt(    r.x) * cos(2.0*pi*r.z)
        );
}

// https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Quaternion-derived_rotation_matrix
mat3 quaternion_to_matrix_original(vec4 q) // https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
{
    float s = dot(q,q); // just in case, not needed for uniform quaternions
    return
        mat3(1) // identity
        +
        2.0
        *
        mat3
        (
            vec3(-q.y*q.y-q.z*q.z,+q.x*q.y-q.z*q.w,+q.x*q.z+q.y*q.w),
            vec3(+q.x*q.y+q.z*q.w,-q.x*q.x-q.z*q.z,+q.y*q.z-q.x*q.w),
            vec3(+q.x*q.z-q.y*q.w,+q.y*q.z+q.x*q.w,-q.x*q.x-q.y*q.y)
        )
        /
        s;
}

mat3 quaternion_to_matrix(vec4 q) // testing alternative
{
    vec3 u = q.xyz;
    float w = q.w;

    mat3 u_outer = outerProduct(u, u); // u * u^T
    mat3 u_cross =
        mat3
        (
            vec3(   0, -u.z,  u.y),
            vec3( u.z,    0, -u.x),
            vec3(-u.y,  u.x,    0)
        );

    float s = dot(q,q);

    return (mat3(w*w - dot(u,u)) + 2.0 * (u_outer + w * u_cross)) / s;
}

mat3 random_rotation(vec3 r)
{
    return quaternion_to_matrix(random_unit_quaternion(r));
}

bool shape_single(ivec3 voxel)
{
    if(length(vec3(voxel)) > 10.0*float(N)) return false;
#ifdef GYROID
    return gyroid(vec3(voxel)/20.0) < 0.0;
#endif
#ifdef NOISE
    return noise(vec3(voxel)/10.0) < 0.0;//+vec3(voxel).z*0.2+0.5 < 0.0;
#endif
    return true;
}

bool shape(ivec3 voxel)
{
    for(int z = 0; z <= +1; z++)
    {
        for(int y = 0; y <= +1; y++)
        {
            for(int x = 0; x <= +1; x++)
            {
                if(shape_single(voxel+ivec3(x,y,z))) return true; // ANY neighbor filled?
            }
        }
    }

    return false;
}

struct result
{
    vec3 color;
    float dist;
};

result combine(result a, result b)
{
    if(a.dist < b.dist) return a;
    return b;
}

uint uhash(uvec3 v)
{
    return HASH(v.x + HASH(v.y + HASH(v.z)));
}

vec3 rainbow(float t)
{
    return 0.5-0.5*cos((t+vec3(0,1,2)/3.0)*tau);
}

result item(ivec3 voxel, vec3 local)
{
    if(!shape_single(voxel)) return result(vec3(0), DIST_MAX);

    vec3 color = rainbow(length(vec3(voxel))*0.1);

    vec3 r = hash33(vec3(voxel)*123.456);

    // TODO: uncorrelate rotation from colors
    mat3 R = random_rotation(r.xyz); // NOTE: r must be in [0,1]

    return result(color, box(R*(local-0.5), vec3(sqrt(0.40)))); // XXX guessed size XXX
}

result map(ivec3 voxel, vec3 local)
{
    result r = result(vec3(0), DIST_MAX); // signal to skip this voxel

    for(int z = 0; z <= +1; z++)
    {
        for(int y = 0; y <= +1; y++)
        {
            for(int x = 0; x <= +1; x++)
            {
                r = combine(r, item(voxel+ivec3(x,y,z), local-vec3(x,y,z)+0.5)); // NOTE: 2^3 voxels meeting at 0.5
            }
        }
    }

    return r;
}

result map(vec3 p)
{
    ivec3 voxel = ivec3(floor(p));
    vec3 local = fract(p);

    if(shape(voxel))
        return map(voxel, local);

    return result(vec3(0), DIST_MAX);
}

vec3 normal(ivec3 voxel, vec3 p) // https://iquilezles.org/articles/normalsSDF tetrahedron normals
{
    const float h = 0.001; // normal epsilon
    const vec2 k = vec2(1,-1);
    return normalize(k.xyy*map(voxel, p + k.xyy*h).dist +
                     k.yyx*map(voxel, p + k.yyx*h).dist +
                     k.yxy*map(voxel, p + k.yxy*h).dist +
                     k.xxx*map(voxel, p + k.xxx*h).dist);
}

// avoid loop unrolling to reduce compile-time on windows
vec3 normal(vec3 p) // https://iquilezles.org/articles/normalsSDF/
{
    const float h = 0.0001;
    vec3 n = vec3(0);
    for( int i=ZERO; i<4; i++ )
    {
        vec3 e = (2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0)/sqrt(3.0);
        n += e*map(p+e*h).dist;
    }
    return normalize(n);
}

result raymarch(ivec3 voxel, vec3 ro, vec3 rd, float t0, float t1, bool pass)
{
    const float raymarch_epsilon = 0.001;
    result h;
    int i;
    float t;
    for(t = t0, i = ZERO; t < t1 && i < MAX_MARCH_STEPS; i++) // finite loop originally suggested by pyBlob to avoid stalling if ray parallel to surface just above EPSILON
    {
        vec3 p = ro + rd * t;
        h = map(voxel, p);
        if(h.dist < raymarch_epsilon)
            return result(h.color, t);

        t += h.dist;
    }

    if(!pass)
        return result(h.color, t); // stop on running out of iterations

    return result(vec3(0), t1); // pass on running out of iterations
}

float maxcomp(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

// NOTE: Apparently sign fails on some systems! Thanks to spalmer for debugging this!
vec3 sgn(vec3 v) // WORKAROUND FOR COMPILER ERROR on some systems
{
    return step(vec3(0), v) * 2.0 - 1.0;
}

result traverse_voxels(vec3 ray_pos, vec3 ray_dir, bool pass) // https://www.shadertoy.com/view/W3lSDf dda voxel steps without division
{
    ivec3 voxel = ivec3(floor(ray_pos));
    bvec3 nearest = bvec3(0);

    for(int i = ZERO; i < MAX_VOXEL_STEPS; i++)
    {
        vec3 offset = ray_pos - vec3(voxel) - vec3(0.5);
        vec3 side_offset = 0.5 - sgn(ray_dir) * offset;

        if(shape(voxel))
        {
            vec3 color0 = vec3(1,1,0)*dot(vec3(0.5,1.0,0.75),vec3(nearest));
            float d0 = maxcomp(vec3(nearest) * (side_offset - 1.0) / abs(ray_dir)); // robust when if multiple equally near

            vec3 code = side_offset * abs(ray_dir.yzx * ray_dir.zxy); // which side is nearest? NOTE: avoiding div zero by multiplying by all components and cancelling (jt)
            bvec3 nearest = lessThanEqual(code.xyz, min(code.yzx, code.zxy)); // mark smallest components

            vec3 side_offset = 0.5 - (sgn(ray_dir) * offset - vec3(nearest));

            vec3 color1 = vec3(0,0,1)*dot(vec3(0.5,1.0,0.75),vec3(nearest));
            float d1 = maxcomp(vec3(nearest) * (side_offset - 1.0) / abs(ray_dir)); // robust when if multiple equally near

            const float margin = 0.001; // WARNING: too high margin destroys shadows!
            result r = raymarch(voxel, offset+0.5, ray_dir, d0-margin, d1+margin, pass);
            r.dist = max(d0, r.dist); // clamp minimal distance to voxel (removes "cloud-of-flies" artifact)
            if(r.dist < d1+margin) return r; // hit
        }

        vec3 code = side_offset * abs(ray_dir.yzx * ray_dir.zxy); // which side is nearest? NOTE: avoiding div zero by multiplying by all components and cancelling (jt)
        nearest = lessThanEqual(code.xyz, min(code.yzx, code.zxy)); // mark smallest components
        voxel += ivec3(vec3(nearest) * sgn(ray_dir)); // advance to next voxel
    }

    return result(vec3(0), DIST_MAX); // miss
}

float ambient_occlusion(vec3 pos, in vec3 nor) // https://www.shadertoy.com/view/Xds3zN raymarching primitives by iq
{
    float occ = 0.0;
    float sca = 1.0;
    for(int i = 0; i<5; i++)
    {
        float h = 0.01 + 0.12*float(i)/4.0;
        float d = map( pos + h*nor ).dist;
        occ += (h-d)*sca;
        sca *= 0.95;
        if( occ>0.35 ) break;
    }
    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 ) ;
}

void mainImage(out vec4 color_alpha, in vec2 I)
{
    bool demo = all(lessThan(iMouse.xy, vec2(10.0)));
    vec2 R = iResolution.xy;
    I = (I + I - R) / R.y; // concise pixel-position mapping thanks to Fabrice
#ifdef SPEEDUP_CHEAT
    if(length(I) > 1.0) { color_alpha = vec4(0); return; }
#endif
    float yaw = 2.0 * pi * float(iMouse.x) / float(R.x);
    float pitch = pi - pi / 2.0 * float(iMouse.y) / float(R.y);
    yaw = !demo ? yaw : 2.0 * pi * fract(iTime * 0.01);
    pitch = !demo ? pitch : pi*2.0/3.0;

    vec3 ray_pos = vec3(0.0, 0.0,-25.0*float(N));
    vec3 ray_dir = vec3(I.x, I.y, 2.0); // NOTE: normalize not necessary here

    mat3 M = yaw_pitch_roll(yaw, pitch, 0.0);
    ray_pos = M * ray_pos;
    ray_dir = M * ray_dir;

    ray_dir = normalize(ray_dir);

    vec3 light_dir = normalize(vec3(0,0,-3));
    light_dir = M * light_dir; // camera-light
    float ambient = 0.1;

    vec3 sky_color = vec3(0);
    vec3 color = vec3(0);
    result r = traverse_voxels(ray_pos, ray_dir, false);
    if(r.dist < DIST_MAX)
    {
        color = r.color;
        vec3 surf = ray_pos + ray_dir * r.dist;
        vec3 norm = normal(surf);

        float ao = ambient_occlusion(surf, norm);
        float diffuse = max(0.0, dot(light_dir, norm));

        if(diffuse > 0.0)
        {
            if(traverse_voxels(surf+norm*0.01, light_dir, true).dist < DIST_MAX)
                diffuse = 0.0;
        }

        color *= (ambient * ao + diffuse);

        if(diffuse > 0.0)
        {
            float specular = pow(max(0.0, dot(norm, normalize(-ray_dir + light_dir))), 250.0);
            color += specular;
        }

        vec3 fog_color = sky_color;
        color = mix(fog_color, vec3(color), exp(-pow(r.dist/50.0/float(N), 2.0))); // fog
    }
    else
    {
        color = sky_color;
    }

    color = tanh(color); // roll-off overly bright colors
    color = sqrt(color); // approximate gamma

    color_alpha = vec4(color,1);
}
```

## Algorithm

Transcribe the reference top-to-bottom. The DDA traversal, SDF, rotation math, lighting, and AO stay unchanged. Only the input parameters, the cut test, the per-voxel color source, the camera control, and the final tone pipeline change.

### Keep verbatim

- `hash12`, `hash13`, `hash32`, `hash33` (Dave_Hoskins Hash-without-Sine).
- `triple32`, `lowbias32`, `HASH` macro, `uhash(uvec2)`, `uhash(uvec3)`.
- `grad`, `noise` (3D Perlin) - used only when `cutMode == NOISE`.
- `gyroid` - used only when `cutMode == GYROID`.
- `box` SDF.
- `random_unit_quaternion`, `quaternion_to_matrix` (keep the compact alternative form, drop the `_original` function), `random_rotation`.
- `yaw_pitch_roll` rotation matrix builder.
- `sgn` (sign workaround).
- `maxcomp`.
- `combine`.
- `result` struct (color + dist).
- `traverse_voxels` DDA loop - every line, including the duplicate `nearest` shadowing and the `margin = 0.001` constant.
- `raymarch` finite SDF stepper with `raymarch_epsilon = 0.001`.
- Both `normal` overloads (voxel-local and global) - the shading path uses the global one; keep the voxel-local overload available for completeness.
- `ambient_occlusion` (5 samples, `sca *= 0.95`, break at `occ > 0.35`).
- `shape_single` neighbor loop structure (the 2x2x2 sampling is non-negotiable per the reference author - it's what makes the cube rotation produce continuous seams).
- `shape` 2x2x2 OR-reduction.
- `item`, `map(voxel, local)`, `map(vec3 p)` structure (only the `color =` and `box(R*..., vec3(cubeSize))` lines change).
- `ZERO` macro (`min(0, frame)`) to prevent loop unrolling.
- `SPEEDUP_CHEAT` circular mask - keep it unconditionally; the cluster is bounded, pixels outside the disc never hit geometry.
- `MAX_VOXEL_STEPS = 128`, `MAX_MARCH_STEPS = 100`, `DIST_MAX = 200.0` (bake the N=2 defaults directly - do not expose N).

### Replace

| Original | Replacement | Reason |
|----------|-------------|--------|
| `iTime` | `time` uniform | CPU-accumulated phase |
| `iResolution` | `resolution` uniform | Standard naming |
| `iFrame` | `frame` uniform (int) | Needed for `ZERO` macro |
| `iMouse` | Remove entirely | Not applicable |
| `#define GYROID` / `#define NOISE` | `int cutMode` uniform (0=GYROID, 1=NOISE), runtime branch in `shape_single` | Both modes selectable at runtime |
| `length(voxel) > 10.0*float(N)` | `length(vec3(voxel)) > clusterRadius` | Parameterized cluster size |
| `gyroid(vec3(voxel)/20.0)` | `gyroid(vec3(voxel) * cutScale + fieldOffset)` | Parameterized cut-field scale + CPU-accumulated drift |
| `noise(vec3(voxel)/10.0)` | `noise(vec3(voxel) * cutScale + fieldOffset)` | Same `cutScale` uniform works for both (different defaults in the two config presets) |
| `< 0.0` threshold | `< cutThresholdBase + cutThresholdPulse * sin(cutPulsePhase)` | Pulsing threshold |
| `sqrt(0.40)` box half-extent | `cubeSize` uniform, default `sqrt(0.40)` (~0.632) | Parameterized fullness |
| `rainbow(length(vec3(voxel))*0.1)` | `texture(gradientLUT, vec2(fract(length(vec3(voxel)) * colorRate), 0.5)).rgb` | Gradient LUT sampled by radial distance |
| `hash33(vec3(voxel)*123.456)` rotation seed | keep unchanged | Reference uses the same seed for color; we decouple by using `gradientLUT` for color instead - the rotation hash stays put |
| `yaw = 2.0 * pi * fract(iTime * 0.01)` | `yaw = time * orbitSpeed` (CPU-accumulated) | Parameterized orbit rate |
| `pitch = pi*2.0/3.0` | `pitch = orbitPitch` uniform, default `2.0 * PI / 3.0` | Parameterized viewing angle |
| `ray_pos = vec3(0, 0, -25*N)` | `ray_pos = vec3(0.0, 0.0, -cameraDistance)` | Parameterized camera distance |
| `ray_dir = vec3(I.x, I.y, 2.0)` | `ray_dir = vec3(I.x, I.y, focalLength)`, with `focalLength` baked as `2.0` constant | No need to expose - just inline `2.0` |
| `float ambient = 0.1` local | `ambient` uniform | Parameterized |
| `250.0` specular power | `specularPower` uniform | Parameterized glint tightness |
| `r.dist/50.0/float(N)` fog falloff | `r.dist / fogDistance` | Parameterized, default 25.0 |
| `color = tanh(color)` soft clip | **Remove** | Tonemap forbidden per project rules; output HDR |
| `color = sqrt(color)` gamma | **Remove** | Gamma handled by output `Gamma` effect |
| `color_alpha = vec4(color, 1)` | Apply blend compositor with standard `blendIntensity`, `blendMode` | Matches generator pattern |

### Add

| Addition | Purpose |
|----------|---------|
| `gradientLUT` sampler2D | Standard gradient LUT for radial color |
| `colorRate` uniform | Scales `length(voxel) * colorRate` wrap frequency |
| `cutMode` int uniform | 0=GYROID, 1=NOISE switch |
| `cutScale` float uniform | Cut-field wavelength |
| `cutThresholdBase` float | Static threshold offset |
| `cutThresholdPulse` float | Threshold oscillation amplitude |
| `cutPulsePhase` float (CPU-accumulated from `cutPulseSpeed`) | Passed as uniform |
| `fieldOffset` vec3 uniform (CPU-accumulated from `fieldDriftX/Y/Z`) | Translates sample position into the cut field over time |
| `orbitPhase` float uniform (CPU-accumulated from `orbitSpeed`) | Passed as `yaw` into the camera matrix |
| `orbitPitch` float uniform | Camera pitch angle |
| `cameraDistance` float uniform | Distance from origin to ray origin (pre-rotation) |
| `clusterRadius` float uniform | Spherical cluster bound |
| `cubeSize` float uniform | Box SDF half-extent |
| `ambient` float uniform | Base ambient lighting |
| `specularPower` float uniform | Specular exponent (higher = tighter glint) |
| `fogDistance` float uniform | Distance at which fog reaches ~1/e opacity |
| `fftTexture` sampler2D | FFT spectral data |
| `sampleRate` float uniform | FFT bin mapping |
| `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright` | Standard generator FFT params |
| Standard generator output: `EffectBlendMode blendMode`, `float blendIntensity`, `ColorConfig gradient` | Matches generator pattern via `STANDARD_GENERATOR_OUTPUT` |

### Coordinate convention note

The reference's `I = (I + I - R) / R.y` already centers `I` on the screen (range roughly `[-aspect, aspect]` horizontally, `[-1, 1]` vertically). Map this to AudioJones's convention: `I = (fragTexCoord * resolution - resolution * 0.5) / (resolution.y * 0.5)`. Do not use raw `fragTexCoord` for anything - it is bottom-left-origin UV.

### CPU phase accumulation (per conventions)

In `GeodeSetup()`:

```
e->cutPulsePhase += cfg->cutPulseSpeed * deltaTime;
e->orbitPhase += cfg->orbitSpeed * deltaTime;
e->fieldOffset.x += cfg->fieldDriftX * deltaTime;
e->fieldOffset.y += cfg->fieldDriftY * deltaTime;
e->fieldOffset.z += cfg->fieldDriftZ * deltaTime;
```

All `*Speed` / `*Drift*` fields are CPU-accumulated phases. The shader only sees the resulting offsets. Never call `sin(time * speed)` in the shader.

### FFT injection: radial-band emission

In `item()`, after the `color = texture(gradientLUT, ...)` line, modulate the color by an FFT band selected by radial distance from center:

```
float radialT = clamp(length(vec3(voxel)) / clusterRadius, 0.0, 1.0);
float freq = baseFreq * pow(maxFreq / baseFreq, radialT);
float bin = freq / (sampleRate * 0.5);
float energy = baseBright + pow(clamp(texture(fftTexture, vec2(bin, 0.5)).r * gain, 0.0, 1.0), curve);
color *= energy;
```

Center cubes get low-frequency reactivity; outer-shell cubes get high-frequency. `baseBright` is the minimum floor so silent audio still renders.

### Gyroid vs noise default `cutScale`

The reference divides by 20 for gyroid and 10 for noise, giving different visual densities. Default `cutScale`:

- Gyroid mode: `0.05` (= 1/20)
- Noise mode: `0.1` (= 1/10)

The user edits a single `cutScale`. When switching modes, the visual density will look different at the same slider value - this is fine; the user adjusts after switching. Do not auto-swap defaults on mode change.

### Removed optimizations to NOT re-add

- Do not reintroduce `tanh` / `sqrt` post-processing. Output is linear HDR; the pipeline's output stage handles gamma.
- Do not reintroduce mouse-based camera control. The camera is time-driven only.
- Do not reintroduce the `PREVIEW` / `SHOWITER` style dev toggles. Single render path.

## Parameters

### Cluster

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| clusterRadius | float | 5.0-25.0 | 20.0 | Spherical bound; cubes only spawn inside this radius |
| cubeSize | float | 0.3-0.7 | 0.632 | Per-cube SDF half-extent (reference's `sqrt(0.40)`) |
| colorRate | float | 0.02-0.5 | 0.1 | How fast gradient LUT cycles along radial distance |

### Cut Field

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| cutMode | int (combo) | {GYROID, NOISE} | GYROID | Which 3D field carves the cluster |
| cutScale | float | 0.02-0.25 | 0.05 | Cut-field wavelength (smaller = larger caverns) |
| cutThresholdBase | float | -2.0-2.0 | 0.0 | Static threshold offset (positive = more filled, negative = more carved) |
| cutThresholdPulse | float | 0.0-2.0 | 0.0 | Threshold oscillation amplitude (0 = no pulse) |
| cutPulseSpeed | float | 0.0-4.0 | 0.5 | Threshold oscillation frequency (Hz) |
| fieldDriftX | float | -2.0-2.0 | 0.0 | Cut-field drift rate on X axis (units/sec) |
| fieldDriftY | float | -2.0-2.0 | 0.0 | Cut-field drift rate on Y axis (units/sec) |
| fieldDriftZ | float | -2.0-2.0 | 0.0 | Cut-field drift rate on Z axis (units/sec) |

### Camera

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| orbitSpeed | float | -1.0-1.0 | 0.06 | Yaw orbit rate (radians/sec); matches reference's `2*pi*0.01` |
| orbitPitch | float | 0.0 - PI | 2.094 (2PI/3) | Fixed pitch angle (radians) |
| cameraDistance | float | 30.0-80.0 | 50.0 | Camera ray-origin Z distance |

### Lighting

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| ambient | float | 0.0-0.5 | 0.1 | Baseline lighting outside direct illumination |
| specularPower | float | 10.0-1000.0 | 250.0 | Specular exponent (higher = tighter, gemmier glint) |
| fogDistance | float | 10.0-100.0 | 25.0 | Distance at which fog reaches ~1/e opacity |

### Audio

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT band frequency |
| maxFreq | float | 1000.0-16000.0 | 14000.0 | Highest FFT band frequency |
| gain | float | 0.1-10.0 | 2.0 | FFT energy amplification |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum emission floor (cubes still render when silent) |

### Output (standard generator)

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| gradient (ColorConfig) | embedded | - | standard | Radial color palette |
| blendIntensity | float | 0.0-1.0 | 1.0 | Generator blend amount |
| blendMode | enum | - | ADD | Composite mode |

## Modulation Candidates

- **cutThresholdBase**: Globally opens/closes caverns. Dramatic negative values shred the cluster; positive packs it solid.
- **cutThresholdPulse**: Breathing amplitude. Brings the cluster in and out of carving every cycle.
- **cutPulseSpeed**: How fast the breathing happens.
- **fieldDriftX/Y/Z**: Directional flow of caverns through the cluster. Diagonal vectors create wave motion.
- **cutScale**: Cavern grain. Low values = vast caverns with few cubes showing; high values = fine salt grains.
- **clusterRadius**: Cluster scale. Modulating this pulses the outer envelope.
- **cubeSize**: Cube fullness. Small = dust cloud; large = packed rock.
- **colorRate**: Color stripe frequency along radial distance.
- **orbitSpeed**: Orbit rate. Modulating from zero kicks the camera into motion on beats.
- **specularPower**: Glint tightness. Low = wide glare like wet rock; high = pinpoint gemstone sparkles.
- **fogDistance**: Near/far falloff. Low = tight misty view; high = everything crisp.
- **ambient**: Baseline brightness in shadowed areas.

### Interaction Patterns

**fieldDrift (magnitude) x cutThresholdPulse (resonance)**: Both affect how the carved interior reorganizes. Both near zero = static frozen rock. Both active = massive churning rearrangement with caverns opening and sliding at once. Routing `drift` magnitude from sub-bass and `cutThresholdPulse` from kick transient creates rare alignment where drops cause the rock to visually detonate.

**cubeSize x cutThresholdBase (competing forces)**: `cubeSize` expands each cube's SDF outward while `cutThresholdBase` decides which voxel cells exist at all. Small cubes + high threshold = floating dust grains. Large cubes + low threshold = packed solid that barely reveals caverns. Routing these from opposing sources (bass vs treble) produces tension that makes verses look crumbly and choruses look monolithic (or vice versa).

**orbitSpeed x fieldDriftX (parallax resonance)**: When camera orbit and field drift align in direction and rate, caverns appear locked to the view (frozen parallax). When they diverge, caverns drift across the cube shell independently of camera motion, producing depth cues. Rare alignment moments feel like the world stops.

**cutPulseSpeed x orbitSpeed (frequency alignment)**: Both are temporal rates. When they divide evenly (e.g., 0.5 vs 0.25), the visual rhythm syncs with the orbit. When they're coprime, the scene never repeats twice the same way at the same viewing angle.

## Notes

- **Performance**: This is heavy. Every rendered pixel does up to 128 DDA voxel steps, each with a 2x2x2=8 neighbor `shape` query. Every voxel hit does an inner SDF raymarch (up to 100 steps) that sums the SDFs of 8 rotated cubes. Every shaded pixel does an AO pass (5 SDF samples) and potentially a full second DDA traverse for self-shadow. Expect significant GPU cost on 1080p+. The `SPEEDUP_CHEAT` disc mask discards ~21% of pixels for free since the cluster is bounded.
- **Compile time on Windows**: The reference author notes loop-unrolling concerns. Keep the `ZERO = min(0, frame)` macro in every loop bound (AO, normals, DDA step cap, inner raymarch). Without it, nested loops will unroll and inflate compile time.
- **GLSL 330 compatibility**: All features used (bitwise uint ops, `outerProduct`, `lessThanEqual`, `bvec3`, `ivec3`, `floor`/`fract` on vec3, `tanh` in `exp` for fog) are available in GLSL 330. Note: the reference's `tanh` + `sqrt` post-processing is removed; no tonemap in shader.
- **Shadow cost**: The self-shadow pass casts a second `traverse_voxels` per lit pixel. This is the single most expensive thing in the shader. If performance is unacceptable, the cleanest perf cut is to replace `diffuse = 0` on shadow-hit with `diffuse *= 0.5` and skip the inner traverse entirely - but that sacrifices the deep-shadow contact that sells the 3D structure.
- **Shape seam continuity**: The 2x2x2 neighbor sampling in `shape()` and `map()` is what makes randomly-rotated cubes blend smoothly at cell boundaries. Reducing to 1x1 sampling produces gap artifacts; reference author tried 3x3x3 and found 2x2x2 sufficient. Do not change.
- **Color seed decoupling**: Reference uses the same `hash33(vec3(voxel)*123.456)` for both rotation and (commented-out) color hash. Our gradient LUT samples by `length(voxel)` instead, so rotation randomness and color are naturally decoupled.
- **`cubeSize` sweet spot**: Values below ~0.5 start showing inter-cube gaps at every seam. Values above ~0.65 produce SDF self-intersection artifacts between neighbors. Default 0.632 (reference value) is close to the practical max.
