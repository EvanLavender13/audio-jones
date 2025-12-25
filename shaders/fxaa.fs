#version 330

// FXAA 3.11 - Fast Approximate Anti-Aliasing
// Based on Timothy Lottes' FXAA algorithm from NVIDIA

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;

out vec4 finalColor;

// FXAA quality settings
#define FXAA_EDGE_THRESHOLD_MIN 0.0312
#define FXAA_EDGE_THRESHOLD 0.125
#define FXAA_SUBPIX_QUALITY 0.75
#define FXAA_ITERATIONS 12

// Luminance weights (perceptual)
float Luminance(vec3 rgb)
{
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

void main()
{
    vec2 texelSize = 1.0 / resolution;

    // Sample center and 4-neighborhood
    vec3 rgbM = texture(texture0, fragTexCoord).rgb;
    vec3 rgbN = texture(texture0, fragTexCoord + vec2(0.0, -texelSize.y)).rgb;
    vec3 rgbS = texture(texture0, fragTexCoord + vec2(0.0, texelSize.y)).rgb;
    vec3 rgbE = texture(texture0, fragTexCoord + vec2(texelSize.x, 0.0)).rgb;
    vec3 rgbW = texture(texture0, fragTexCoord + vec2(-texelSize.x, 0.0)).rgb;

    // Calculate luminance
    float lumM = Luminance(rgbM);
    float lumN = Luminance(rgbN);
    float lumS = Luminance(rgbS);
    float lumE = Luminance(rgbE);
    float lumW = Luminance(rgbW);

    // Find min/max luminance in neighborhood
    float lumMin = min(lumM, min(min(lumN, lumS), min(lumE, lumW)));
    float lumMax = max(lumM, max(max(lumN, lumS), max(lumE, lumW)));
    float lumRange = lumMax - lumMin;

    // Skip anti-aliasing if contrast is too low
    if (lumRange < max(FXAA_EDGE_THRESHOLD_MIN, lumMax * FXAA_EDGE_THRESHOLD))
    {
        finalColor = vec4(rgbM, 1.0);
        return;
    }

    // Sample diagonal neighbors
    vec3 rgbNW = texture(texture0, fragTexCoord + vec2(-texelSize.x, -texelSize.y)).rgb;
    vec3 rgbNE = texture(texture0, fragTexCoord + vec2(texelSize.x, -texelSize.y)).rgb;
    vec3 rgbSW = texture(texture0, fragTexCoord + vec2(-texelSize.x, texelSize.y)).rgb;
    vec3 rgbSE = texture(texture0, fragTexCoord + vec2(texelSize.x, texelSize.y)).rgb;

    float lumNW = Luminance(rgbNW);
    float lumNE = Luminance(rgbNE);
    float lumSW = Luminance(rgbSW);
    float lumSE = Luminance(rgbSE);

    // Compute edge direction
    float lumNS = lumN + lumS;
    float lumWE = lumW + lumE;
    float lumNWSW = lumNW + lumSW;
    float lumNENE = lumNE + lumSE;
    float lumNWNE = lumNW + lumNE;
    float lumSWSE = lumSW + lumSE;

    // Determine if edge is horizontal or vertical
    float edgeH = abs(lumNWSW - 2.0 * lumW) + abs(lumNS - 2.0 * lumM) * 2.0 + abs(lumNENE - 2.0 * lumE);
    float edgeV = abs(lumNWNE - 2.0 * lumN) + abs(lumWE - 2.0 * lumM) * 2.0 + abs(lumSWSE - 2.0 * lumS);
    bool isHorizontal = edgeH >= edgeV;

    // Select edge endpoints based on direction
    float lum1 = isHorizontal ? lumN : lumW;
    float lum2 = isHorizontal ? lumS : lumE;
    float grad1 = lum1 - lumM;
    float grad2 = lum2 - lumM;

    bool is1Steeper = abs(grad1) >= abs(grad2);
    float gradScaled = 0.25 * max(abs(grad1), abs(grad2));

    // Step size along edge
    float stepLength = isHorizontal ? texelSize.y : texelSize.x;
    float lumLocalAvg = 0.0;

    if (is1Steeper)
    {
        stepLength = -stepLength;
        lumLocalAvg = 0.5 * (lum1 + lumM);
    }
    else
    {
        lumLocalAvg = 0.5 * (lum2 + lumM);
    }

    // Shift UV to edge
    vec2 currentUV = fragTexCoord;
    if (isHorizontal)
    {
        currentUV.y += stepLength * 0.5;
    }
    else
    {
        currentUV.x += stepLength * 0.5;
    }

    // Step along edge in both directions
    vec2 offset = isHorizontal ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);
    vec2 uv1 = currentUV - offset;
    vec2 uv2 = currentUV + offset;

    float lumEnd1 = Luminance(texture(texture0, uv1).rgb) - lumLocalAvg;
    float lumEnd2 = Luminance(texture(texture0, uv2).rgb) - lumLocalAvg;

    bool reached1 = abs(lumEnd1) >= gradScaled;
    bool reached2 = abs(lumEnd2) >= gradScaled;
    bool reachedBoth = reached1 && reached2;

    if (!reached1) { uv1 -= offset; }
    if (!reached2) { uv2 += offset; }

    // Continue searching along edge
    if (!reachedBoth)
    {
        for (int i = 2; i < FXAA_ITERATIONS; i++)
        {
            if (!reached1)
            {
                lumEnd1 = Luminance(texture(texture0, uv1).rgb) - lumLocalAvg;
            }
            if (!reached2)
            {
                lumEnd2 = Luminance(texture(texture0, uv2).rgb) - lumLocalAvg;
            }

            reached1 = abs(lumEnd1) >= gradScaled;
            reached2 = abs(lumEnd2) >= gradScaled;
            reachedBoth = reached1 && reached2;

            if (!reached1) { uv1 -= offset; }
            if (!reached2) { uv2 += offset; }

            if (reachedBoth) { break; }
        }
    }

    // Calculate distances to edge endpoints
    float dist1 = isHorizontal ? (fragTexCoord.x - uv1.x) : (fragTexCoord.y - uv1.y);
    float dist2 = isHorizontal ? (uv2.x - fragTexCoord.x) : (uv2.y - fragTexCoord.y);

    bool isDir1 = dist1 < dist2;
    float distFinal = min(dist1, dist2);
    float edgeLength = dist1 + dist2;

    // Subpixel anti-aliasing
    float pixelOffset = -distFinal / edgeLength + 0.5;

    // Check if edge direction matches luminance gradient
    bool isLumMSmaller = lumM < lumLocalAvg;
    bool correctVariation = ((isDir1 ? lumEnd1 : lumEnd2) < 0.0) != isLumMSmaller;
    float finalOffset = correctVariation ? pixelOffset : 0.0;

    // Subpixel aliasing test
    float lumAvg = (1.0 / 12.0) * (2.0 * (lumNS + lumWE) + lumNWSW + lumNENE);
    float subPixOffset = clamp(abs(lumAvg - lumM) / lumRange, 0.0, 1.0);
    float subPixOffsetFinal = (-2.0 * subPixOffset + 3.0) * subPixOffset * subPixOffset;
    subPixOffsetFinal = subPixOffsetFinal * subPixOffsetFinal * FXAA_SUBPIX_QUALITY;

    finalOffset = max(finalOffset, subPixOffsetFinal);

    // Final sample with offset
    vec2 finalUV = fragTexCoord;
    if (isHorizontal)
    {
        finalUV.y += finalOffset * stepLength;
    }
    else
    {
        finalUV.x += finalOffset * stepLength;
    }

    finalColor = vec4(texture(texture0, finalUV).rgb, 1.0);
}
