// Based on "大龙猫 - Quicky#009" by totetmatt
// https://www.shadertoy.com/view/wdt3zn
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced diamond distance field with input brightness displacement,
//           added per-channel color displacement, parameterized all constants

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float phase;
uniform float stripeCount;
uniform float stripeWidth;
uniform float displacement;
uniform float channelSpread;
uniform float colorDisplace;
uniform float hardEdge;

void main() {
    vec4 src = texture(texture0, fragTexCoord);
    float lum = dot(src.rgb, vec3(0.299, 0.587, 0.114));

    // Per-channel displacement: blend between shared luminance and per-channel RGB
    float dispR = mix(lum, src.r, colorDisplace);
    float dispG = mix(lum, src.g, colorDisplace);
    float dispB = mix(lum, src.b, colorDisplace);

    // Hard edge: 0=continuous displacement, 1=fully binary step at 0.5 threshold
    dispR = mix(dispR, step(0.5, dispR), hardEdge);
    dispG = mix(dispG, step(0.5, dispG), hardEdge);
    dispB = mix(dispB, step(0.5, dispB), hardEdge);

    // Displaced Y positions per channel
    float yR = fragTexCoord.y + dispR * displacement;
    float yG = fragTexCoord.y + dispG * displacement;
    float yB = fragTexCoord.y + dispB * displacement;

    // Per-channel stripes with phase separation
    // R: cos, base speed
    // G: sin (90 degree base offset), slightly slower
    // B: cos with +1.0 offset, slightly faster
    float r = 1.0 - step(stripeWidth, abs(cos(-phase + yR * stripeCount)));
    float g = 1.0 - step(stripeWidth, abs(sin(-phase * (1.0 - channelSpread) + yG * stripeCount)));
    float b = 1.0 - step(stripeWidth, abs(cos(1.0 + phase * (1.0 + channelSpread) + yB * stripeCount)));

    finalColor = vec4(r, g, b, 1.0);
}
