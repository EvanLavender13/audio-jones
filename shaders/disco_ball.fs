#version 330

// Disco ball: faceted mirror sphere reflecting input texture

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float sphereRadius;      // Ball size as fraction of screen height
uniform float tileSize;          // Facet grid density (smaller = more tiles)
uniform float sphereAngle;       // Accumulated rotation (radians)
uniform float bumpHeight;        // Edge bevel depth
uniform float reflectIntensity;  // Brightness multiplier

// Light projection uniforms
uniform float spotIntensity;      // Background light spot brightness
uniform float spotSize;           // Cutoff angle (higher = smaller spots)
uniform float spotFalloff;        // Spot edge softness (higher = softer)
uniform float brightnessThreshold; // Minimum input brightness to project

out vec4 finalColor;

const float PI = 3.14159265359;
const float TAU = 6.28318530718;
const int LAT_STEPS = 16;
const int LON_STEPS = 8;

// Ray-sphere intersection for centered sphere
// Returns t (ray distance) or -1.0 on miss
float RaySphere(vec3 ro, vec3 rd, float radius) {
    float b = dot(ro, rd);
    float c = dot(ro, ro) - radius * radius;
    float h = b * b - c;
    if (h < 0.0) return -1.0;
    return -b - sqrt(h);
}

void main()
{
    // Map UV to normalized device coords centered at origin
    vec2 p = -1.0 + 2.0 * fragTexCoord;
    p.x *= resolution.x / resolution.y;

    // Ray setup: perspective (matches reference)
    vec3 ro = vec3(0.0, 0.0, -2.5);
    vec3 rd = normalize(vec3(p, 2.0));

    float t = RaySphere(ro, rd, sphereRadius);

    if (t < 0.0) {
        // Miss - project light spots from facets
        vec3 bgColor = vec3(0.0);

        if (spotIntensity > 0.0) {
            // Direction from sphere center to this background pixel
            vec3 toPixel = normalize(vec3(p, 0.5));

            // Iterate subsampled facets for light projection
            for (int lat = 0; lat < LAT_STEPS; lat++) {
                for (int lon = 0; lon < LON_STEPS; lon++) {
                    // Facet center in spherical coords
                    float theta = (float(lat) + 0.5) / float(LAT_STEPS) * PI;
                    float phi = (float(lon) + 0.5) / float(LON_STEPS) * TAU + sphereAngle;

                    // Facet normal (sphere centered at origin)
                    vec3 facetNormal = vec3(
                        sin(theta) * cos(phi),
                        cos(theta),
                        sin(theta) * sin(phi)
                    );

                    // Skip back-facing facets (facing away from camera at +Z)
                    if (facetNormal.z < 0.0) continue;

                    // Sample reflected color from input texture
                    vec3 facetRefl = reflect(vec3(0.0, 0.0, 1.0), facetNormal);
                    vec2 facetUV = facetRefl.xy * 0.5 + 0.5;
                    vec3 reflectedColor = texture(texture0, facetUV).rgb;
                    float brightness = dot(reflectedColor, vec3(0.299, 0.587, 0.114));

                    // Skip dim facets
                    if (brightness < brightnessThreshold) continue;

                    // Alignment: does facet point toward this pixel?
                    float alignment = dot(facetNormal, toPixel);

                    if (alignment > spotSize) {
                        float spot = pow((alignment - spotSize) / (1.0 - spotSize), spotFalloff);
                        bgColor += reflectedColor * spot * spotIntensity;
                    }
                }
            }
        }

        finalColor = vec4(bgColor, 1.0);
        return;
    }

    // Hit position on sphere surface
    vec3 pos = ro + rd * t;

    // Convert to spherical coordinates (phi = longitude, theta = latitude)
    vec2 phiTheta = vec2(
        atan(pos.z, pos.x) + sphereAngle,
        acos(clamp(pos.y / sphereRadius, -1.0, 1.0))
    );

    // Quantize into facet grid
    vec2 tileId = floor(phiTheta / tileSize);
    vec2 tilePos = (phiTheta - tileId * tileSize) / tileSize;

    // Convert tile corner back to world space (reference approach)
    phiTheta = tileId * tileSize;
    phiTheta.x -= sphereAngle;

    // Edge darkening at tile boundaries
    vec2 edge = min(clamp(tilePos * 10.0, 0.0, 1.0), clamp((1.0 - tilePos) * 10.0, 0.0, 1.0));
    float bump = clamp(min(edge.x, edge.y), 0.0, 1.0);

    // Reconstruct facet position on sphere for normal calculation
    vec3 bumpPos;
    bumpPos.x = sphereRadius * sin(phiTheta.y) * cos(phiTheta.x);
    bumpPos.y = sphereRadius * cos(phiTheta.y);
    bumpPos.z = sphereRadius * sin(phiTheta.y) * sin(phiTheta.x);
    bumpPos.y += (1.0 - bump) * bumpHeight;

    vec3 normal = normalize(bumpPos);

    // Compute reflection vector
    vec3 refl = reflect(rd, normal);

    // Sample texture using reflection (simple mapping)
    vec3 color = texture(texture0, refl.xy * 0.5 + 0.5).rgb;

    // Square for contrast
    color = color * color;

    // Darken edges, boost intensity
    color *= bump * reflectIntensity;

    // Gamma correction
    color = pow(color, vec3(0.6));

    finalColor = vec4(color, 1.0);
}
