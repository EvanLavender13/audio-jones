// Attractor Lines: 12 RK4-integrated strange attractor trails (one per semitone)
// with FFT-gated brightness. Each particle's state persists in pixels (0..11, 0).
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D previousFrame;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;

uniform int attractorType;
uniform float sigma;
uniform float rho;
uniform float beta;
uniform float rosslerC;
uniform float thomasB;
uniform float dadrasA;
uniform float dadrasB;
uniform float dadrasC;
uniform float dadrasD;
uniform float dadrasE;

uniform float steps;
uniform float speed;
uniform float viewScale;
uniform float intensity;
uniform float decayFactor;
uniform float focus;
uniform float maxSpeed;
uniform float x;
uniform float y;
uniform mat3 rotationMatrix;

uniform float sampleRate;
uniform float baseFreq;
uniform int numOctaves;
uniform float gain;
uniform float curve;
uniform float baseBright;

// Per-attractor tuning constants
// Step sizes tuned for smooth segments at each system's velocity
const float STEP_LORENZ  = 0.006;  // Fast dynamics, small dt for smooth curves
const float STEP_ROSSLER = 0.02;
const float STEP_AIZAWA  = 0.04;   // Small derivatives, needs larger dt
const float STEP_THOMAS  = 0.15;   // Very slow dynamics
const float STEP_DADRAS  = 0.012;

// Spatial normalization so all attractors fill similar screen area
// Matches attractor_agents.glsl projectToScreen() per-attractor scaling
const float SCALE_LORENZ  = 1.0;
const float SCALE_ROSSLER = 1.0;
const float SCALE_AIZAWA  = 8.0;
const float SCALE_THOMAS  = 4.0;
const float SCALE_DADRAS  = 2.7;

// Reset threshold — position beyond this means the integrator diverged
const float DIVERGE_LORENZ  = 200.0;
const float DIVERGE_ROSSLER = 200.0;
const float DIVERGE_AIZAWA  = 10.0;
const float DIVERGE_THOMAS  = 20.0;
const float DIVERGE_DADRAS  = 100.0;

float getStepSize(int type) {
    if (type == 0) return STEP_LORENZ;
    if (type == 1) return STEP_ROSSLER;
    if (type == 2) return STEP_AIZAWA;
    if (type == 3) return STEP_THOMAS;
    return STEP_DADRAS;
}

float getScaleFactor(int type) {
    if (type == 0) return SCALE_LORENZ;
    if (type == 1) return SCALE_ROSSLER;
    if (type == 2) return SCALE_AIZAWA;
    if (type == 3) return SCALE_THOMAS;
    return SCALE_DADRAS;
}

float getDivergeLimit(int type) {
    if (type == 0) return DIVERGE_LORENZ;
    if (type == 1) return DIVERGE_ROSSLER;
    if (type == 2) return DIVERGE_AIZAWA;
    if (type == 3) return DIVERGE_THOMAS;
    return DIVERGE_DADRAS;
}

vec3 getCenterOffset(int type) {
    if (type == 0) return vec3(0.0, 0.0, 27.0); // Lorenz
    return vec3(0.0);
}

vec3 getStartingPoint(int pid) {
    float angle = float(pid) * 6.28318 / 12.0;
    float r = 1.5;
    return vec3(r * cos(angle), r * sin(angle), r * sin(angle * 0.7 + 1.0));
}

vec3 derivativeLorenz(vec3 p) {
    return vec3(
        sigma * (p.y - p.x),
        p.x * (rho - p.z) - p.y,
        p.x * p.y - beta * p.z
    );
}

vec3 derivativeRossler(vec3 p) {
    return vec3(
        -p.y - p.z,
        p.x + 0.2 * p.y,
        0.2 + p.z * (p.x - rosslerC)
    );
}

vec3 derivativeAizawa(vec3 p) {
    float zShift = p.z - 0.7;
    float r2 = p.x * p.x + p.y * p.y;
    return vec3(
        zShift * p.x - 3.5 * p.y,
        3.5 * p.x + zShift * p.y,
        0.6 + 0.95 * p.z - p.z * p.z * p.z / 3.0
            - r2 * (1.0 + 0.25 * p.z) + 0.1 * p.z * p.x * p.x * p.x
    );
}

vec3 derivativeThomas(vec3 p) {
    return vec3(
        sin(p.y) - thomasB * p.x,
        sin(p.z) - thomasB * p.y,
        sin(p.x) - thomasB * p.z
    );
}

vec3 derivativeDadras(vec3 p) {
    return vec3(
        p.y - dadrasA * p.x + dadrasB * p.y * p.z,
        dadrasC * p.y - p.x * p.z + p.z,
        dadrasD * p.x * p.y - dadrasE * p.z
    );
}

vec3 attractorDerivative(vec3 p, int type) {
    if (type == 0) return derivativeLorenz(p);
    if (type == 1) return derivativeRossler(p);
    if (type == 2) return derivativeAizawa(p);
    if (type == 3) return derivativeThomas(p);
    return derivativeDadras(p);
}

// RK4 integration step — matches attractor_agents.glsl pattern
vec3 rk4Step(vec3 p, float dt, int type) {
    vec3 k1 = attractorDerivative(p, type);
    vec3 k2 = attractorDerivative(p + 0.5 * dt * k1, type);
    vec3 k3 = attractorDerivative(p + 0.5 * dt * k2, type);
    vec3 k4 = attractorDerivative(p + dt * k3, type);
    return p + (dt / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
}

float dfLine(vec2 a, vec2 b, vec2 p) {
    vec2 ab = b - a;
    float len2 = dot(ab, ab);
    if (len2 < 1e-10) return distance(a, p);
    float t = clamp(dot(p - a, ab) / len2, 0.0, 1.0);
    return distance(a + ab * t, p);
}

vec2 project(vec3 p, int type) {
    p -= getCenterOffset(type);
    p = rotationMatrix * p;
    float s = viewScale * getScaleFactor(type);
    // Match attractor_agents.glsl projection planes per attractor
    if (type == 0) return vec2(p.x, p.z) * s;  // Lorenz: XZ
    if (type == 2) return vec2(p.x, p.z) * s;  // Aizawa: XZ
    return p.xy * s;                             // Rossler, Thomas, Dadras: XY
}

void main() {
    vec2 res = resolution / resolution.y;
    vec2 uv = fragTexCoord * res - res / 2.0;

    vec2 offset = vec2(x - 0.5, y - 0.5) * res;
    float dt = getStepSize(attractorType) * speed;
    float divergeLimit = getDivergeLimit(attractorType);
    int numSteps = clamp(int(steps), 1, 256);

    // State persistence — 12 pixels in row 0
    if (gl_FragCoord.y < 1.0 && gl_FragCoord.x < 12.0) {
        int px = int(gl_FragCoord.x);
        vec3 pos = texelFetch(previousFrame, ivec2(px, 0), 0).xyz;
        if (dot(pos, pos) < 1e-10 || any(isnan(pos)))
            pos = getStartingPoint(px);

        for (int i = 0; i < numSteps; i++) {
            vec3 next = rk4Step(pos, dt, attractorType);
            if (length(next) > divergeLimit) {
                next = getStartingPoint(px);
            }
            pos = next;
        }

        finalColor = vec4(pos, 1.0);
        return;
    }

    // Distance field — nested particle x step loop
    float d = 1e6;
    float bestSpeed = 0.0;
    int winnerIdx = 0;

    for (int pid = 0; pid < 12; pid++) {
        vec3 pos = texelFetch(previousFrame, ivec2(pid, 0), 0).xyz;
        if (dot(pos, pos) < 1e-10 || any(isnan(pos)))
            pos = getStartingPoint(pid);

        vec2 projLast = project(pos, attractorType) + offset;

        for (int i = 0; i < numSteps; i++) {
            vec3 next = rk4Step(pos, dt, attractorType);

            if (length(next) > divergeLimit) {
                next = getStartingPoint(pid);
                pos = next;
                projLast = project(pos, attractorType) + offset;
                continue;
            }

            vec2 projNext = project(next, attractorType) + offset;
            float segD = dfLine(projLast, projNext, uv);
            if (segD < d) {
                d = segD;
                bestSpeed = length(attractorDerivative(next, attractorType));
                winnerIdx = pid;
            }

            pos = next;
            projLast = projNext;
        }
    }

    // FFT-gated brightness for winning particle's semitone
    float mag = 0.0;
    for (int oct = 0; oct < numOctaves; oct++) {
        float freq = baseFreq * pow(2.0, float(winnerIdx) / 12.0 + float(oct));
        float bin = freq / (sampleRate * 0.5);
        if (bin <= 1.0)
            mag += texture(fftTexture, vec2(bin, 0.5)).r;
    }
    mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    float c = intensity * smoothstep(focus / resolution.y, 0.0, d);
    c += (intensity / 8.5) * exp(-1000.0 * d * d);

    float speedNorm = clamp(bestSpeed / maxSpeed, 0.0, 1.0);
    vec3 color = texture(gradientLUT, vec2(speedNorm, 0.5)).rgb;

    vec3 prev = texelFetch(previousFrame, ivec2(gl_FragCoord.xy), 0).rgb;
    finalColor = vec4(color * c * brightness + prev * decayFactor, 1.0);
}
