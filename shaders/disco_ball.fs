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
    float aspect = resolution.x / resolution.y;
    vec2 uv = fragTexCoord - 0.5;
    uv.x *= aspect;

    // Ray setup: orthographic front view
    vec3 ro = vec3(uv, -2.0);
    vec3 rd = vec3(0.0, 0.0, 1.0);

    float t = RaySphere(ro, rd, sphereRadius);

    if (t < 0.0) {
        // Miss - pass through original texture
        finalColor = texture(texture0, fragTexCoord);
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
    vec2 tilePos = fract(phiTheta / tileSize);

    // Compute facet center in spherical coords
    vec2 tileCenter = (tileId + 0.5) * tileSize;

    // Reconstruct facet center position on sphere
    vec3 facetPos;
    facetPos.x = sin(tileCenter.y) * cos(tileCenter.x - sphereAngle);
    facetPos.y = cos(tileCenter.y);
    facetPos.z = sin(tileCenter.y) * sin(tileCenter.x - sphereAngle);

    // Edge bump: darker at tile boundaries
    vec2 edge = min(tilePos * 10.0, (1.0 - tilePos) * 10.0);
    float bump = clamp(min(edge.x, edge.y), 0.0, 1.0);

    // Perturb normal at edges for bevel effect
    facetPos.y += (1.0 - bump) * bumpHeight;
    vec3 normal = normalize(facetPos);

    // Compute reflection vector
    vec3 refl = reflect(rd, normal);

    // Map 3D reflection to 2D texture coords
    vec2 reflUV = refl.xy * 0.5 + 0.5;
    vec3 color = texture(texture0, reflUV).rgb;

    // Darken edges, boost intensity
    color *= bump * reflectIntensity;

    finalColor = vec4(color, 1.0);
}
