// shaders/pitch_spiral.fs
#version 330

// Pitch Spiral: Archimedean spiral maps FFT bins to musical pitch,
// coloring each note via gradient LUT indexed by pitch class.

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float baseFreq;
uniform int numTurns;
uniform float spiralSpacing;
uniform float lineWidth;
uniform float blur;
uniform float gain;
uniform float curve;
uniform float tilt;
uniform float tiltAngle;

#define TAU 6.28318530718
#define TET_ROOT 1.05946309436

void main() {
    // =========================================
    // STEP 1: Spiral geometry
    // =========================================

    vec2 uv;
    if (tilt > 0.0) {
        // Perspective tilt: rotate by tiltAngle, apply Cosmic-style matrix + depth
        vec2 r = resolution;
        vec2 centered = fragTexCoord * r - r * 0.5;
        float ca = cos(tiltAngle), sa = sin(tiltAngle);
        vec2 rotated = vec2(centered.x * ca - centered.y * sa,
                            centered.x * sa + centered.y * ca);
        mat2 tiltMat = mat2(1.0, -tilt, 2.0 * tilt, 1.0 + tilt);
        vec2 p = rotated * tiltMat;
        float depth = r.y + r.y - p.y * tilt;
        uv = p / depth;
    } else {
        uv = fragTexCoord - 0.5;
        uv.x *= resolution.x / resolution.y;
    }

    float angle = atan(uv.y, uv.x);       // [-PI, PI]
    float radius = length(uv);

    // Archimedean spiral: offset increases with radius and wraps with angle
    float offset = radius + (angle / TAU) * spiralSpacing;
    float whichTurn = floor(offset / spiralSpacing);

    // =========================================
    // STEP 2: Pitch mapping
    // =========================================

    // Convert spiral position to cents above baseFreq
    float cents = (whichTurn - angle / TAU) * 1200.0;

    // Convert cents to frequency, then to normalized FFT bin
    float freq = baseFreq * pow(TET_ROOT, cents / 100.0);
    float bin = freq / (sampleRate * 0.5);

    // Clamp: bins beyond Nyquist or beyond visible octaves render black
    float magnitude = 0.0;
    if (bin <= 1.0 && bin >= 0.0 && whichTurn >= 0.0 && whichTurn < float(numTurns)) {
        magnitude = texture(fftTexture, vec2(bin, 0.5)).r;
    }

    // Amplify then shape contrast — gain lifts raw FFT values into visible range
    magnitude = clamp(magnitude * gain, 0.0, 1.0);
    magnitude = pow(magnitude, curve);

    // =========================================
    // STEP 3: Drawing
    // =========================================

    // Spiral line mask: distance from band center within each turn
    float bandPos = mod(offset, spiralSpacing);
    float bandCenter = spiralSpacing * 0.5;
    float dist = abs(bandPos - bandCenter);

    // Soft-edged line using smoothstep on half-width
    float halfWidth = lineWidth * 0.5;
    float lineAlpha = smoothstep(halfWidth + blur, halfWidth - blur, dist);

    // Color via gradient LUT indexed by pitch class (fractional octave)
    float pitchClass = fract(cents / 1200.0);
    vec4 noteColor = texture(gradientLUT, vec2(pitchClass, 0.5));

    // =========================================
    // STEP 4: Final output
    // =========================================

    finalColor = vec4(noteColor.rgb * magnitude * lineAlpha, 1.0);
}
