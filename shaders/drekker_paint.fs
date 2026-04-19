// Based on "Max Drekker Paint effect v2" by Cotterzz
// https://www.shadertoy.com/view/33SSDm
// Fork of "Max Drekker Paint effect" by Cotterzz
// https://shadertoy.com/view/WX2SWz
// License: CC BY-NC-SA 3.0

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float xDiv;
uniform float yDiv;
uniform float curve;
uniform float gapSize;
uniform float diagSlant;
uniform float strokeSpread;

const float REFERENCE_HEIGHT = 400.0;

vec4 sampleCell(vec2 fragCoord) {
    vec2 uv = fragCoord / resolution.y;
    float aspect = resolution.x / resolution.y;
    float rx = floor(uv.x * xDiv);
    float rix = fract(uv.x * xDiv);
    float rixl = (curve / rix) - curve;
    float rixr = (curve / (1.0 - rix)) - curve;

    float ry = floor((uv.y + rx * diagSlant) * yDiv + rixl - rixr);
    float riy = fract((uv.y + rx * diagSlant) * yDiv + rixl - rixr) - 0.5;
    riy = riy * riy * riy;

    vec2 sampleUV = vec2(rx / (xDiv * aspect),
                         (riy * strokeSpread / REFERENCE_HEIGHT) + (ry / yDiv) - rx * diagSlant);
    vec4 c = textureLod(texture0, sampleUV, 0.0);

    if (abs(riy) > 0.125 * (1.0 - gapSize) || rix > (1.0 - gapSize) || rix < gapSize) {
        c.a = 0.0;
    }
    return c;
}

void main() {
    vec2 fragCoord = fragTexCoord * resolution;
    vec2 offset = vec2(0.5);
    vec4 accum = vec4(0.0);
    for (int k = 0; k < 4; k++) {
        accum += sampleCell(fragCoord + offset - 0.5);
        offset = fract(offset + vec2(0.569840296, 0.754877669));
    }
    accum /= 4.0;
    finalColor = vec4(accum.rgb * accum.a, 1.0);
}
