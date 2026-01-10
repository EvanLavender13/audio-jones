#version 330

// Lattice Fold: Grid-based tiling symmetry (square, hexagon)

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform int cellType;       // Cell geometry: 4=square, 6=hexagon
uniform float cellScale;    // Cell density (1.0-20.0)
uniform float rotation;     // Pattern rotation (radians)
uniform float time;         // Animation time (seconds)

out vec4 finalColor;

const float TWO_PI = 6.28318530718;
const float SQRT3 = 1.7320508;
const vec2 HEX_S = vec2(1.0, SQRT3);

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
        // Square: abs() gives 4-fold, then apply radial fold for kaleidoscope
        cell = getSquare(cellUV);
        local = cell.xy;
        float cellAngle = atan(local.y, local.x);
        float segAngle = TWO_PI / 4.0;
        cellAngle = mod(cellAngle, segAngle);
        cellAngle = min(cellAngle, segAngle - cellAngle);
        float cellR = length(local);
        local = vec2(cos(cellAngle), sin(cellAngle)) * cellR;
    } else {
        // Hexagon: apply 6-fold radial symmetry
        cell = getHex(cellUV);
        local = cell.xy;
        float cellAngle = atan(local.y, local.x);
        float segAngle = TWO_PI / 6.0;
        cellAngle = mod(cellAngle, segAngle);
        cellAngle = min(cellAngle, segAngle - cellAngle);
        float cellR = length(local);
        local = vec2(cos(cellAngle), sin(cellAngle)) * cellR;
    }

    // Map back to texture coords
    vec2 newUV = local / cellScale + 0.5;

    finalColor = texture(texture0, clamp(newUV, 0.0, 1.0));
}
