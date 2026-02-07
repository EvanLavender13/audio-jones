// Raymarched volumetric particle trails — thin luminous filaments spiraling
// through 3D space like charged particles in a bubble chamber.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform int marchSteps;
uniform int turbulenceOctaves;
uniform float turbulenceStrength;
uniform float ringThickness;
uniform float cameraDistance;
uniform float colorFreq;
uniform float colorSpeed;
uniform float brightness;
uniform float exposure;
uniform sampler2D gradientLUT;

void main() {
    // Screen coords centered at origin, matching original's (I+I - res.xyy) pattern
    vec2 fragCoord = fragTexCoord * resolution;
    vec3 rayDir = normalize(vec3(fragCoord * 2.0, 0.0) - vec3(resolution.x, resolution.y, resolution.y));

    float z = 0.0;
    float s = 0.0;
    float d = 0.0;
    vec3 color = vec3(0.0);

    for (int i = 0; i < marchSteps; i++) {
        // Sample point along ray, camera offset in z only
        vec3 p = z * rayDir;
        p.z += cameraDistance;

        // Time-varying rotation axis — s from previous step breaks periodicity
        vec3 a = normalize(cos(vec3(7.0, 1.0, 0.0) + time - s));

        // Rodrigues rotation of sample point around axis
        a = a * dot(a, p) - cross(a, p);

        // FBM turbulence: layered sine displacement with .yzx swizzle
        // Original: for(d=1.;d++<9.;) a+=sin(a*d+t).yzx/d
        d = 1.0;
        for (int j = 1; j < turbulenceOctaves; j++) {
            d += 1.0;
            a += sin(a * d + time).yzx / d * turbulenceStrength;
        }

        // Shell distance in warped space
        s = length(a);
        d = ringThickness * abs(sin(s));

        // Adaptive step — smaller near shells for sharp crossings
        z += d;

        // Accumulate color — guard division to prevent NaN from true zero d*s
        float lutCoord = fract(z * colorFreq + time * colorSpeed);
        vec3 sampleColor = textureLod(gradientLUT, vec2(lutCoord, 0.5), 0.0).rgb;
        color += sampleColor / max(d * s, 1e-6);
    }

    // Soft HDR rolloff
    color = tanh(color * brightness / exposure);

    finalColor = vec4(color, 1.0);
}
