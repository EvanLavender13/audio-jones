#version 330

// Lattice Fold: Grid-based tiling symmetry (square, hexagon)

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform int cellType;       // Cell geometry: 4=square, 6=hexagon
uniform float cellScale;    // Cell density (1.0-20.0)
uniform float rotation;     // Pattern rotation (radians)
uniform float time;         // Animation time (seconds)
uniform float smoothing;    // Blend width at radial fold seams (0.0-0.5)

out vec4 finalColor;

const float TWO_PI = 6.28318530718;
const float SQRT3 = 1.7320508;
const vec2 HEX_S = vec2(1.0, SQRT3);

// Polynomial smooth min (for smooth folding)
float pmin(float a, float b, float k)
{
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

// Polynomial smooth absolute (for soft wedge edges)
float pabs(float a, float k)
{
    return -pmin(a, -a, k);
}

// 2D rotation
vec2 rotate2d(vec2 p, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

// Get hex cell coordinates: xy = local position, zw = cell ID
vec4 getHex(vec2 p)
{
    vec4 hC = floor(vec4(p, p - vec2(0.5, 1.0)) / HEX_S.xyxy) + 0.5;
    vec4 h = vec4(p - hC.xy * HEX_S, p - (hC.zw + 0.5) * HEX_S);
    return dot(h.xy, h.xy) < dot(h.zw, h.zw)
        ? vec4(h.xy, hC.xy)
        : vec4(h.zw, hC.zw + 0.5);
}

// Get square cell coordinates: xy = local position, zw = cell ID
vec4 getSquare(vec2 p)
{
    vec2 cell = floor(p);
    vec2 local = fract(p) - 0.5;
    local = abs(local);  // 4-fold symmetry
    return vec4(local, cell);
}

void main()
{
    vec2 uv = fragTexCoord - 0.5;

    // Apply rotation
    uv = rotate2d(uv, rotation);

    // Scale and animate
    vec2 cellUV = uv * cellScale + time * 0.2;

    // Get cell coordinates based on type
    vec4 cell;
    vec2 local;

    if (cellType == 4) {
        cell = getSquare(cellUV);
    } else {
        cell = getHex(cellUV);
    }
    local = cell.xy;

    // Apply N-fold radial symmetry (N = cellType) with optional smoothing
    float segAngle = TWO_PI / float(cellType);
    float halfSeg = segAngle * 0.5;
    float cellAngle = atan(local.y, local.x);
    cellAngle = mod(cellAngle, segAngle);

    // Apply smooth fold when smoothing > 0
    if (smoothing > 0.0) {
        float sa = halfSeg - pabs(halfSeg - cellAngle, smoothing);
        cellAngle = sa;
    } else {
        cellAngle = min(cellAngle, segAngle - cellAngle);
    }

    float cellR = length(local);
    local = vec2(cos(cellAngle), sin(cellAngle)) * cellR;

    // Map back to texture coords
    vec2 newUV = local / cellScale + 0.5;

    finalColor = texture(texture0, clamp(newUV, 0.0, 1.0));
}
