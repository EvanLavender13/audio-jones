// Attractor Lines: RK4-integrated strange attractor trails with distance-field glow.
// Reads integration state from pixel (0,0), advances N steps, evaluates per-pixel
// segment distance, blends new lines onto faded previous frame.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D previousFrame;
uniform sampler2D gradientLUT;

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
uniform float viewScale;
uniform float intensity;
uniform float fade;
uniform float focus;
uniform float maxSpeed;
uniform float x;
uniform float y;
uniform mat3 rotationMatrix;

// Per-attractor RK4 step size — tuned for smooth segments at each system's velocity
float getStepSize(int type) {
    if (type == 0) return 0.006;  // Lorenz: fast dynamics, small dt for smooth curves
    if (type == 1) return 0.02;   // Rossler
    if (type == 2) return 0.04;   // Aizawa: small derivatives, needs larger dt
    if (type == 3) return 0.15;   // Thomas: very slow dynamics
    return 0.012;                  // Dadras
}

// Normalizes spatial extent so all attractors fill similar screen area
// Matches attractor_agents.glsl projectToScreen() per-attractor scaling
float getScaleFactor(int type) {
    if (type == 0) return 1.0;    // Lorenz: no extra scale in simulation
    if (type == 1) return 1.0;    // Rossler: no extra scale in simulation
    if (type == 2) return 8.0;    // Aizawa: simulation uses 8x
    if (type == 3) return 4.0;    // Thomas: simulation uses 4x
    return 2.7;                    // Dadras: no simulation counterpart
}

// Reset threshold — position beyond this means the integrator diverged
float getDivergeLimit(int type) {
    if (type == 0) return 200.0;
    if (type == 1) return 200.0;
    if (type == 2) return 10.0;
    if (type == 3) return 20.0;
    return 100.0;
}

vec3 getCenterOffset(int type) {
    if (type == 0) return vec3(0.0, 0.0, 27.0); // Lorenz
    return vec3(0.0);
}

vec3 getStartingPoint(int type) {
    if (type == 0) return vec3(1.0, 1.0, 1.0);   // Lorenz
    if (type == 1) return vec3(1.0, 1.0, 0.0);   // Rossler
    if (type == 2) return vec3(0.1, 0.0, 0.0);   // Aizawa
    if (type == 3) return vec3(1.0, 1.1, 1.2);   // Thomas
    return vec3(1.0, 1.0, 1.0);                   // Dadras
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
    float t = clamp(dot(p - a, ab) / dot(ab, ab), 0.0, 1.0);
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

    vec3 last = texelFetch(previousFrame, ivec2(0, 0), 0).xyz;
    if (dot(last, last) < 1e-10 || any(isnan(last)))
        last = getStartingPoint(attractorType);

    vec2 offset = vec2(x - 0.5, y - 0.5) * res;
    float dt = getStepSize(attractorType);
    float divergeLimit = getDivergeLimit(attractorType);

    float d = 1e6;
    float bestSpeed = 0.0;
    int numSteps = clamp(int(steps), 1, 256);
    vec3 next;

    for (int i = 0; i < numSteps; i++) {
        next = rk4Step(last, dt, attractorType);

        if (length(next) > divergeLimit) {
            next = getStartingPoint(attractorType);
            last = next;
            continue;
        }

        float segD = dfLine(
            project(last, attractorType) + offset,
            project(next, attractorType) + offset,
            uv
        );
        if (segD < d) {
            d = segD;
            bestSpeed = length(attractorDerivative(next, attractorType));
        }

        last = next;
    }

    if (gl_FragCoord.x < 1.0 && gl_FragCoord.y < 1.0) {
        finalColor = vec4(next, 1.0);
        return;
    }

    float c = intensity * smoothstep(focus / resolution.y, 0.0, d);
    c += (intensity / 8.5) * exp(-1000.0 * d * d);

    float speedNorm = clamp(bestSpeed / maxSpeed, 0.0, 1.0);
    vec3 color = texture(gradientLUT, vec2(speedNorm, 0.5)).rgb;

    vec3 prev = texelFetch(previousFrame, ivec2(gl_FragCoord.xy), 0).rgb;
    finalColor = vec4(color * c + prev * fade, 1.0);
}
