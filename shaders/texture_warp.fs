#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float strength;       // Warp magnitude per iteration
uniform int iterations;       // Cascade depth
uniform int channelMode;      // Channel combination for UV offset

// RGB to HSV for Polar mode
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

void main()
{
    vec2 warpedUV = fragTexCoord;

    for (int i = 0; i < iterations; i++) {
        vec3 s = texture(texture0, warpedUV).rgb;
        vec2 offset;

        if (channelMode == 0) {
            // RG
            offset = (s.rg - 0.5) * 2.0;
        } else if (channelMode == 1) {
            // RB
            offset = (s.rb - 0.5) * 2.0;
        } else if (channelMode == 2) {
            // GB
            offset = (s.gb - 0.5) * 2.0;
        } else if (channelMode == 3) {
            // Luminance
            float l = dot(s, vec3(0.299, 0.587, 0.114));
            offset = vec2(l - 0.5) * 2.0;
        } else if (channelMode == 4) {
            // LuminanceSplit
            float l = dot(s, vec3(0.299, 0.587, 0.114));
            offset = vec2(l - 0.5, 0.5 - l) * 2.0;
        } else if (channelMode == 5) {
            // Chrominance
            offset = vec2(s.r - s.b, s.g - s.b) * 2.0;
        } else {
            // Polar (mode 6)
            vec3 hsv = rgb2hsv(s);
            float angle = hsv.x * 6.28318530718;
            offset = vec2(cos(angle), sin(angle)) * hsv.y;
        }

        warpedUV += offset * strength;
    }

    finalColor = texture(texture0, clamp(warpedUV, 0.0, 1.0));
}
