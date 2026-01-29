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

out vec4 finalColor;

const float PI = 3.14159265359;

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
        // Miss - black background
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
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
