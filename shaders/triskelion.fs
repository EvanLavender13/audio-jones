// Based on "Triskelion" by rrrola
// https://www.shadertoy.com/view/dl3SRr
// License: CC BY-NC-SA 3.0 Unported
// Modified: AudioJones generator with gradient LUT, FFT reactivity, fold mode uniform
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform int foldMode;          // 0=square, 1=hex
uniform int layers;
uniform float circleFreq;
uniform float colorCycles;
uniform float rotationAngle;   // CPU-accumulated rotation phase
uniform float scaleAngle;      // CPU-accumulated scale oscillation phase
uniform float scaleAmount;
uniform float colorPhase;      // CPU-accumulated color cycling phase
uniform float circlePhase;     // CPU-accumulated circle interference phase
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const int BAND_SAMPLES = 4;

// Fold space into a vertex-centered grid.
// Both paths verbatim from reference fold().
vec2 fold(vec2 p) {
  if (foldMode == 0) {
    // Square cells (reference: #ifdef TETRASKELION path)
    return fract(p) - 0.5;
  } else {
    // Hexagonal cells (reference: #else path)
    vec4 m = vec4(2.0, -1.0, 0.0, sqrt(3.0));
    p.y += m.w / 3.0;
    vec2 t = mat2(m) * p;
    return p - 0.5 * mat2(m.xzyw) * round((floor(t) + ceil(t.x + t.y)) / 3.0);
  }
}

void main() {
  // Centered coordinates (reference: (2.0*fragCoord - iResolution.xy) / iResolution.y)
  vec2 p = fragTexCoord * 2.0 - 1.0;
  p.x *= resolution.x / resolution.y;

  // Reference: float d = 0.5*length(p)
  float d = 0.5 * length(p);

  // Reference: mat2(cos(t),sin(t),-sin(t),cos(t)) * (1.0 - 0.1*cos(t2))
  // Replace: t -> rotationAngle, 0.1 -> scaleAmount, t2 -> scaleAngle
  float ca = cos(rotationAngle), sa = sin(rotationAngle);
  mat2 M = mat2(ca, sa, -sa, ca) * (1.0 - scaleAmount * cos(scaleAngle));

  float fLayers = float(layers);
  float logRatio = log(maxFreq / baseFreq);

  // Reference: for (float i = 0.0; i < 24.0; i++)
  // Replace: 24.0 -> layers
  float val = 0.0;
  for (int i = 0; i < layers; i++) {
    float fi = float(i);

    // Reference: p = fold(M * p)  (verbatim)
    p = fold(M * p);

    // Scalar oscillation for interference cancellation (one channel of reference pal())
    float osc = 0.5 + cos(3.0 * (0.01 * fi - d + colorPhase));

    val += osc / cos(d - circlePhase + circleFreq * length(p));
  }

  // Normalize per iteration, square for contrast
  val /= fLayers;
  float brightness = 0.1152 * val * val;

  // Gradient color: ping-pong (no discontinuity), animated by colorPhase
  float t = 1.0 - abs(mod(d * colorCycles - colorPhase, 2.0) - 1.0);
  vec3 col = texture(gradientLUT, vec2(t, 0.5)).rgb;
  // Tonemap: gradient color at normal brightness, washes to white at singularities
  finalColor = vec4(1.0 - exp(-col * brightness), 1.0);
}
