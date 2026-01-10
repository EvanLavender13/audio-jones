#version 330

// Poincare Disk Hyperbolic Tiling
// Warps texture through hyperbolic space using Mobius translation and fundamental domain folding

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform int tileP;
uniform int tileQ;
uniform int tileR;
uniform vec2 translation;
uniform float rotation;
uniform float diskScale;

out vec4 finalColor;

const float PI = 3.14159265359;

// Mobius translation in hyperbolic disk
// Translates viewpoint; points near edge compress, center expands
vec2 mobiusTranslate(vec2 v, vec2 z) {
    float vv = dot(v, v);
    float zv = dot(z, v);
    float zz = dot(z, z);
    return ((1.0 + 2.0 * zv + zz) * v + (1.0 - vv) * z) /
           (1.0 + 2.0 * zv + vv * zz);
}

// Compute fundamental domain mirrors from triangle angles pi/P, pi/Q, pi/R
void solveFundamentalDomain(int P, int Q, int R,
                            out vec2 A, out vec2 B, out vec3 C) {
    A = vec2(1.0, 0.0);
    B = vec2(-cos(PI / float(P)), sin(PI / float(P)));

    mat2 m = inverse(mat2(A, B));
    vec2 cDir = vec2(cos(PI / float(R)), cos(PI / float(Q))) * m;

    float S2 = 1.0 / (dot(cDir, cDir) - 1.0);
    float S = sqrt(abs(S2));
    C = vec3(cDir * S, abs(S2));
}

// Fold point into fundamental triangular domain
vec2 hyperbolicFold(vec2 z, vec2 A, vec2 B, vec3 C) {
    for (int i = 0; i < 50; i++) {
        int folds = 0;

        // Reflect across line A
        float tA = dot(z, A);
        if (tA < 0.0) {
            z -= 2.0 * tA * A;
            folds++;
        }

        // Reflect across line B
        float tB = dot(z, B);
        if (tB < 0.0) {
            z -= 2.0 * tB * B;
            folds++;
        }

        // Invert through circle C (center C.xy, radius^2 C.z)
        vec2 zc = z - C.xy;
        float d2 = dot(zc, zc);
        if (d2 < C.z) {
            z = C.xy + zc * C.z / d2;
            folds++;
        }

        if (folds == 0) break;
    }
    return z;
}

void main()
{
    // Map to disk coordinates centered at origin
    // Divide by diskScale so larger values make the disk fill more of the screen
    vec2 z = (fragTexCoord - 0.5) * 2.0 / diskScale;

    // Apply Euclidean rotation (hyperbolic isometry)
    float c = cos(rotation), s = sin(rotation);
    z = vec2(c * z.x - s * z.y, s * z.x + c * z.y);

    // Track if outside disk for boundary effect
    float r2 = dot(z, z);
    float outside = step(1.0, r2);

    // Circle inversion: map exterior to interior as mirror image
    if (r2 > 1.0) {
        z = z / r2;
    }

    // Clamp translation to stay inside disk
    vec2 t = translation;
    float tMag2 = dot(t, t);
    if (tMag2 > 0.81) { // 0.9^2
        t *= 0.9 / sqrt(tMag2);
    }

    // Apply Mobius translation
    z = mobiusTranslate(t, z);

    // Construct fundamental domain and fold
    vec2 A, B;
    vec3 C;
    solveFundamentalDomain(tileP, tileQ, tileR, A, B, C);
    z = hyperbolicFold(z, A, B, C);

    // Map back to texture coords
    vec2 uv = z * 0.5 + 0.5;
    vec4 color = texture(texture0, uv);

    // Darken exterior slightly to distinguish from interior
    color.rgb *= mix(1.0, 0.7, outside);

    finalColor = color;
}
