// Based on "Playing marble" by guil (S. Guillitte 2015)
// https://www.shadertoy.com/view/MtX3Ws
// License: CC BY-NC-SA 3.0 Unported
//
// Based on "Nova Marble" by rwvens (Modified from S. Guillitte 2015)
// https://www.shadertoy.com/view/MtdGD8
// License: CC BY-NC-SA 3.0 Unported
//
// Modified: parameterized uniforms, gradient LUT coloring, FFT depth-band brightness,
// animated fold perturbation from Nova Marble

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform float orbitPhase;
uniform float zoom;

uniform int fractalIters;
uniform float foldScale;
uniform float foldOffset;
uniform float trapSensitivity;
uniform float perturbPhase;
uniform float perturbAmp;

uniform int marchSteps;
uniform float stepSize;
uniform float sphereRadius;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

// Keep verbatim from Playing Marble
vec2 csqr(vec2 a) { return vec2(a.x*a.x - a.y*a.y, 2.*a.x*a.y); }

// Keep verbatim from Playing Marble
mat2 rot(float a) {
    return mat2(cos(a), sin(a), -sin(a), cos(a));
}

// Keep verbatim from Playing Marble (ray-sphere intersection from iq)
vec2 iSphere(in vec3 ro, in vec3 rd, in vec4 sph) {
    vec3 oc = ro - sph.xyz;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - sph.w*sph.w;
    float h = b*b - c;
    if (h < 0.0) return vec2(-1.0);
    h = sqrt(h);
    return vec2(-b-h, -b+h);
}

// Nova Marble fold perturbation + Playing Marble fractal core
// Replace: iteration count -> fractalIters uniform
// Replace: fold line -> Nova's cos(perturbPhase) animated version with uniforms
// Replace: trap exponent -> trapSensitivity uniform
// Keep: csqr(p.yz), p=p.zxy axis permutation, res/2 return
float map(in vec3 p) {
    float res = 0.;
    vec3 c = p;
    for (int i = 0; i < fractalIters; ++i) {
        p = foldScale*abs(p + cos(perturbPhase + 1.6)*perturbAmp) / dot(p,p)
            + foldOffset + cos(perturbPhase)*perturbAmp;
        p.yz = csqr(p.yz);
        p = p.zxy;
        res += exp(-trapSensitivity * abs(dot(p,c)));
    }
    return res/2.;
}

// Replace: step count -> marchSteps, base step -> stepSize
// Keep: exp(-2.*c) adaptive slowdown
// Replace: color accumulation with gradient LUT + FFT depth-band brightness
vec3 raymarch(in vec3 ro, vec3 rd, vec2 tminmax) {
    float t = tminmax.x;
    float dt = stepSize;
    vec3 col = vec3(0.);
    float c = 0.;
    for (int i = 0; i < marchSteps; i++) {
        t += dt*exp(-2.*c);
        if (t > tminmax.y) break;

        c = map(ro + t*rd);

        // Normalized depth: 0 at sphere entry, 1 at sphere exit
        float tNorm = (t - tminmax.x) / (tminmax.y - tminmax.x);

        // Gradient LUT color from depth
        vec3 lutColor = texture(gradientLUT, vec2(tNorm, 0.5)).rgb;

        // FFT frequency from depth (bass at front, treble at core)
        float freq = baseFreq * pow(maxFreq / baseFreq, tNorm);
        float bin = freq / (sampleRate * 0.5);
        float energy = 0.0;
        if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
        float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        // Glass body: constant fog fills the sphere so it reads as solid marble
        // even when fractal filaments are sparse at certain perturbation phases
        float glass = 0.015;
        col = 0.99 * col + 0.08 * (c + glass) * lutColor * brightness;
    }
    return col;
}

void main() {
    vec2 q = gl_FragCoord.xy / resolution.xy;
    vec2 p = -1.0 + 2.0 * q;
    p.x *= resolution.x / resolution.y;

    // Camera: orbit only (no mouse), zoom uniform replaces hardcoded 1.0
    vec3 ro = zoom * vec3(4.);
    ro.xz *= rot(orbitPhase);
    vec3 ta = vec3(0.0, 0.0, 0.0);
    vec3 ww = normalize(ta - ro);
    vec3 uu = normalize(cross(ww, vec3(0.0, 1.0, 0.0)));
    vec3 vv = normalize(cross(uu, ww));
    vec3 rd = normalize(p.x*uu + p.y*vv + 4.0*ww);

    // Sphere radius uniform replaces hardcoded 2.0
    vec2 tmm = iSphere(ro, rd, vec4(0., 0., 0., sphereRadius));

    vec3 col = vec3(0.);
    if (tmm.x >= 0.) {
        col = raymarch(ro, rd, tmm);

        // Fresnel edge glow from sphere surface normal
        vec3 nor = (ro + tmm.x * rd) / sphereRadius;
        nor = reflect(rd, nor);
        float fre = pow(0.5 + clamp(dot(nor, rd), 0.0, 1.0), 3.0) * 1.3;
        col += col * fre;
    }

    // Log compression from reference -- not Reinhard, structurally required
    // for 64-step accumulation to resolve filaments without blowout
    col = 0.5 * log(1.0 + col);
    col = clamp(col, 0.0, 1.0);
    finalColor = vec4(col, 1.0);
}
