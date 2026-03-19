// Based on "Icosahedral Party" by OldEclipse
// https://www.shadertoy.com/view/33tSzs
// License: CC BY-NC-SA 3.0 Unported
// Modified: All 5 platonic solids via uniforms, IQ sdCapsule, gradient LUT
// coloring, configurable reflections and camera
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec3 faceNormals[20];
uniform int faceCount;
uniform float planeOffset;
uniform vec3 edgeA[30];
uniform vec3 edgeB[30];
uniform int edgeCount;
uniform float angleXZ;
uniform float angleYZ;
uniform float cameraDistance;
uniform int maxBounces;
uniform float reflectivity;
uniform float edgeRadius;
uniform float edgeGlow;
uniform int maxIterations;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float PI = 3.14159265;

mat2 Rot(float th) {
    float c = cos(th);
    float s = sin(th);
    return mat2(vec2(c, s), vec2(-s, c));
}

float sdPlane(vec3 p, vec3 n, float h) {
    return dot(p, n) + h;
}

float sdCapsule(vec3 p, vec3 a, vec3 b, float r) {
    vec3 pa = p - a;
    vec3 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h) - r;
}

vec2 map(vec3 p) {
    float d = 1e6;
    float type = 0.0;

    for (int i = 0; i < faceCount; i++) {
        d = min(d, sdPlane(p, faceNormals[i], planeOffset));
    }

    float dCap = 1e6;
    for (int i = 0; i < edgeCount; i++) {
        dCap = min(dCap, sdCapsule(p, edgeA[i], edgeB[i], edgeRadius));
    }
    if (dCap < d) {
        d = dCap;
        type = 2.0;
    }

    return vec2(d, type);
}

vec3 getNormal(vec3 p) {
    float d = map(p).x;
    vec2 e = vec2(0.0001, 0.0);
    vec3 n = d - vec3(
        map(p - e.xyy).x,
        map(p - e.yxy).x,
        map(p - e.yyx).x
    );
    return normalize(n);
}

// Find which face normal best matches the surface normal
int findFace(vec3 n) {
    int best = 0;
    float bestDot = -1.0;
    for (int i = 0; i < faceCount; i++) {
        float d = dot(n, faceNormals[i]);
        if (d > bestDot) {
            bestDot = d;
            best = i;
        }
    }
    return best;
}

// Sample FFT energy for a frequency band
float sampleBand(int bandIdx, int totalBands) {
    float freqRatio = maxFreq / baseFreq;
    float freqLo = baseFreq * pow(freqRatio, float(bandIdx) / float(totalBands));
    float freqHi = baseFreq * pow(freqRatio, float(bandIdx + 1) / float(totalBands));
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);

    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    return mix(baseBright, 1.0, mag);
}

const float TARGET_XZ_RATIO = 0.8;
const float TARGET_YZ_RATIO = 1.5;

void main() {
    vec2 uv = (fragTexCoord * 2.0 - 1.0) * vec2(resolution.x / resolution.y, 1.0);

    mat2 R = Rot(angleXZ);
    mat2 R2 = Rot(angleYZ);
    vec3 ro = vec3(0.0, 0.0, -cameraDistance);
    ro.xz = R * ro.xz;
    ro.yz = R2 * ro.yz;

    mat2 R3 = Rot(angleXZ * TARGET_XZ_RATIO);
    mat2 R4 = Rot(angleYZ * TARGET_YZ_RATIO);
    vec3 ta = ro + vec3(1.2 * uv, 1.0);
    ta.xz = R3 * ta.xz;
    ta.yz = R4 * ta.yz;
    vec3 rd = normalize(ta - ro);

    vec3 lightPos = vec3(0.0, 0.4, 0.0);

    float minDist = 0.001;
    float t = 0.0;
    int reflCount = 0;
    vec3 col = vec3(0.0);
    vec3 prevCol = col;

    for (int i = 0; i < maxIterations; i++) {
        vec3 pos = ro + t * rd;
        vec2 m = map(pos);
        float d = m.x;
        float type = m.y;

        if (d < minDist) {
            vec3 n = getNormal(pos);

            if (type == 2.0) {
                col *= edgeGlow;
                if (reflCount > 0) {
                    float p = pow(reflectivity, float(reflCount));
                    col = mix(prevCol, col, p);
                }
                break;
            }

            // Each face maps to a frequency band and gradient position
            int faceIdx = findFace(n);
            float gradT = float(faceIdx) / float(faceCount);
            float fftBrightness = sampleBand(faceIdx, faceCount);
            col = texture(gradientLUT, vec2(gradT, 0.5)).rgb * fftBrightness;

            vec3 l = normalize(lightPos - pos);
            float diffuse = max(dot(n, l), 0.0);
            col = col * (0.5 + 0.5 * diffuse);

            if (reflCount > 0) {
                float p = pow(reflectivity, float(reflCount));
                col = mix(prevCol, col, p);
            }

            prevCol = col;

            if (reflCount >= maxBounces) {
                break;
            }

            ro = pos;
            rd = reflect(rd, n);
            t = 0.01;
            reflCount++;
            continue;
        }

        t += d;
    }

    finalColor = vec4(col, 1.0);
}
