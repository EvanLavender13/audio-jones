#version 330

// Voronoi UV distortion effect
// Warps the image through animated voronoi cell geometry

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float scale;       // Cell count across screen width
uniform float strength;    // UV displacement magnitude
uniform float time;        // Animation driver
uniform float edgeFalloff; // Distortion gradient sharpness
uniform int mode;          // 0=glass blocks, 1=organic flow, 2=edge warp

out vec4 finalColor;

const float TWO_PI = 6.28318530718;

vec2 hash2(vec2 p)
{
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

void main()
{
    if (strength <= 0.0) {
        finalColor = texture(texture0, fragTexCoord);
        return;
    }

    float aspect = resolution.x / resolution.y;
    vec2 st = fragTexCoord * vec2(scale * aspect, scale);
    vec2 ip = floor(st);
    vec2 fp = fract(st);

    // First pass: find nearest cell center
    vec2 mr = vec2(0.0);
    vec2 mg = vec2(0.0);
    float md = 8.0;

    for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
            vec2 g = vec2(float(i), float(j));
            vec2 o = hash2(ip + g);
            o = 0.5 + 0.5 * sin(time + TWO_PI * o);
            vec2 r = g + o - fp;
            float d = dot(r, r);
            if (d < md) {
                md = d;
                mr = r;
                mg = g;
            }
        }
    }

    // Second pass: distance to nearest edge
    float edgeDist = 8.0;
    for (int j = -2; j <= 2; j++) {
        for (int i = -2; i <= 2; i++) {
            vec2 g = mg + vec2(float(i), float(j));
            vec2 o = hash2(ip + g);
            o = 0.5 + 0.5 * sin(time + TWO_PI * o);
            vec2 r = g + o - fp;
            if (dot(mr - r, mr - r) > 0.00001) {
                edgeDist = min(edgeDist, dot(0.5 * (mr + r), normalize(r - mr)));
            }
        }
    }

    // Compute displacement based on mode
    vec2 displacement = vec2(0.0);
    if (mode == 0) {
        // Glass Blocks: hard refraction per cell
        displacement = mr * strength;
    } else if (mode == 1) {
        // Organic Flow: smooth flowing displacement
        displacement = mr * strength * smoothstep(0.0, edgeFalloff, length(mr));
    } else {
        // Edge Warp: distortion at boundaries only
        displacement = mr * strength * (1.0 - smoothstep(0.0, edgeFalloff, edgeDist));
    }

    // Scale displacement back to UV space
    displacement /= vec2(scale * aspect, scale);

    vec2 distortedUV = fragTexCoord + displacement;
    finalColor = texture(texture0, distortedUV);
}
