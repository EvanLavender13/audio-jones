#version 330

// CRT monitor simulation: barrel distortion, scanlines, phosphor mask, pulsing glow, vignette
// Seven stages in fixed order reproduce cathode-ray tube display characteristics

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float time;

uniform int maskMode;               // 0 = shadow mask, 1 = aperture grille
uniform float maskSize;             // Cell pixel size (2.0-24.0)
uniform float maskIntensity;        // Blend strength (0.0-1.0)
uniform float maskBorder;           // Dark gap width (0.0-1.0)

uniform float scanlineIntensity;    // Darkness (0.0-1.0)
uniform float scanlineSpacing;      // Pixels between lines (1.0-8.0)
uniform float scanlineSharpness;    // Transition sharpness (0.5-4.0)
uniform float scanlineBrightBoost;  // Bright pixel resistance (0.0-1.0)

uniform int curvatureEnabled;       // Bool as int
uniform float curvatureAmount;      // Distortion strength (0.0-0.3)

uniform int vignetteEnabled;        // Bool as int
uniform float vignetteExponent;     // Edge falloff curve (0.1-1.0)

uniform int pulseEnabled;           // Bool as int
uniform float pulseIntensity;       // Brightness ripple (0.0-0.1)
uniform float pulseWidth;           // Wavelength in pixels (20.0-200.0)
uniform float pulseSpeed;           // Scroll speed (1.0-40.0)

out vec4 finalColor;

void main()
{
    // Stage 1: Barrel distortion — polar coordinate warp (ported from glitch.fs)
    vec2 uv = fragTexCoord;
    bool outOfBounds = false;
    if (curvatureEnabled != 0) {
        vec2 centered = uv * 2.0 - 1.0;
        float r = length(centered);
        r /= (1.0 - curvatureAmount * r * r);
        float theta = atan(centered.y, centered.x);
        uv = vec2(r * cos(theta), r * sin(theta)) * 0.5 + 0.5;
        outOfBounds = (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0);
    }

    // Stage 2: Sample texture at per-pixel UV (no quantization)
    vec4 color = texture(texture0, uv);

    // Stage 3: Scanlines — sin-based horizontal darkening modulated by brightness
    vec2 pixel = uv * resolution;
    float lum = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    float scanline = sin(pixel.y * 3.14159 / scanlineSpacing);
    scanline = pow(abs(scanline), scanlineSharpness);
    float boost = mix(1.0, lum, scanlineBrightBoost);
    color.rgb *= 1.0 - scanlineIntensity * scanline * boost;

    // Stage 4: Phosphor mask
    if (maskMode == 0) {
        // Shadow mask: staggered rectangular R/G/B subcells
        vec2 coord = pixel / maskSize;
        vec2 subcoord = coord * vec2(3.0, 1.0);

        // Stagger every other column's y-offset
        vec2 cellOffset = vec2(0.0, fract(floor(coord.x) * 0.5));

        // Which RGB channel lights up (0=R, 1=G, 2=B)
        float ind = mod(floor(subcoord.x), 3.0);
        vec3 maskColor = vec3(ind == 0.0 ? 1.0 : 0.0,
                              ind == 1.0 ? 1.0 : 0.0,
                              ind == 2.0 ? 1.0 : 0.0) * 3.0;

        // Soft borders: squared falloff from subcell center
        vec2 cellUV = fract(subcoord + cellOffset) * 2.0 - 1.0;
        vec2 border = 1.0 - cellUV * cellUV * maskBorder;
        maskColor *= border.x * border.y;

        // Blend mask onto image
        color.rgb *= 1.0 + (maskColor - 1.0) * maskIntensity;
    }
    else {
        // Aperture grille: continuous vertical stripes
        float stripe = fract(pixel.x / maskSize * 0.333333);
        float ind = floor(stripe * 3.0);
        vec3 maskColor = vec3(ind == 0.0 ? 1.0 : 0.0,
                              ind == 1.0 ? 1.0 : 0.0,
                              ind == 2.0 ? 1.0 : 0.0) * 3.0;

        // Vertical gap between stripes
        float gap = 1.0 - pow(abs(fract(stripe * 3.0) * 2.0 - 1.0), 2.0) * maskBorder;
        maskColor *= gap;

        color.rgb *= 1.0 + (maskColor - 1.0) * maskIntensity;
    }

    // Stage 5: Pulsing glow — horizontal brightness wave simulating electron beam refresh
    if (pulseEnabled != 0) {
        float pulse = cos(pixel.x / pulseWidth + time * pulseSpeed);
        color.rgb += pulseIntensity * pulse;
    }

    // Stage 6: Vignette (ported from glitch.fs)
    if (vignetteEnabled != 0) {
        float vig = 8.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);
        color.rgb *= pow(vig, vignetteExponent) * 1.5;
    }

    // Stage 7: Bezel clamp — zero out pixels outside barrel distortion bounds
    if (outOfBounds) color = vec4(0.0, 0.0, 0.0, 1.0);

    finalColor = color;
}
