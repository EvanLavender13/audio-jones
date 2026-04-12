# Difference of Gaussians (XDoG)

Subtracts two Gaussian blurs at different scales to isolate edges, then applies soft thresholding to produce clean ink-like contour lines over the image - ranging from delicate pencil outlines to bold manga strokes.

## Classification

- **Category**: TRANSFORMS > Print
- **Pipeline Position**: Transform chain (user-ordered), same slot as Toon

## Attribution

- **Based on**: XDoG isotropic shader by Jan Eric Kyprianidis
- **Source**: https://github.com/jkyprian/flowabs
- **License**: GPL-3.0

## References

- [jkyprian/flowabs](https://github.com/jkyprian/flowabs) - Authoritative GLSL implementation by XDoG paper co-author
- [jkyprian/xdog-demo](https://github.com/jkyprian/xdog-demo) - CUDA reference with all thresholding variants
- [Winnemoller, Kyprianidis, Olsen 2012 - XDoG paper](https://users.cs.northwestern.edu/~sco590/winnemoeller-cag2012.pdf) - Original academic paper

## Reference Code

From jkyprian/flowabs `glsl/dog_fs.glsl` (single-pass isotropic DoG):

```glsl
// by Jan Eric Kyprianidis <www.kyprianidis.com>
uniform sampler2D img;
uniform float sigma_e;
uniform float sigma_r;
uniform float tau;
uniform float phi;
uniform vec2 img_size;

void main() {
    vec2 uv = gl_FragCoord.xy / img_size;
    float twoSigmaESquared = 2.0 * sigma_e * sigma_e;
    float twoSigmaRSquared = 2.0 * sigma_r * sigma_r;
    int halfWidth = int(ceil( 2.0 * sigma_r ));

    vec2 sum = vec2(0.0);
    vec2 norm = vec2(0.0);

    for ( int i = -halfWidth; i <= halfWidth; ++i ) {
        for ( int j = -halfWidth; j <= halfWidth; ++j ) {
            float d = length(vec2(i,j));
            vec2 kernel = vec2( exp( -d * d / twoSigmaESquared ),
                                exp( -d * d / twoSigmaRSquared ));

            vec2 L = texture2D(img, uv + vec2(i,j) / img_size).xx;

            norm += 2.0 * kernel;
            sum += kernel * L;
        }
    }
    sum /= norm;

    float H = 100.0 * (sum.x - tau * sum.y);
    float edge = ( H > 0.0 )? 1.0 : 2.0 * smoothstep(-2.0, 2.0, phi * H );
    gl_FragColor = vec4(vec3(edge), 1.0);
}
```

XDoG thresholding variant (from xdog-demo, tanh version):

```c
float edge = (H > epsilon) ? 1.0 : 1.0 + tanh(phi * (H - epsilon));
```

## Algorithm

Adapt the `dog_fs.glsl` single-pass approach:

- Keep the dual-Gaussian kernel verbatim: compute both `exp(-d^2 / 2*sigma_e^2)` and `exp(-d^2 / 2*sigma_r^2)` per sample
- Keep `sum.x - tau * sum.y` subtraction and `100.0` scaling
- Keep the smoothstep thresholding: `(H > 0) ? 1.0 : 2.0 * smoothstep(-2, 2, phi * H)`
- Replace `texture2D` with `texture()`
- Replace `img_size` with `resolution` uniform
- Replace `gl_FragCoord.xy / img_size` with `fragTexCoord`
- Pixel offset: `vec2(i,j) / resolution`
- The reference samples luminance only (`.xx`). Keep this - DoG operates on grayscale.
- Luminance from input: `dot(color.rgb, vec3(0.299, 0.587, 0.114))` for perceptual luma
- Derive `sigma_r` from `sigma_e * k` in the shader (k fixed at 1.6, the standard LoG approximation ratio)
- Cap `halfWidth` to 12 to prevent GPU stalls
- Multiply edge factor with input color: `inputColor * edge` - gives the original image with dark contour lines along edges

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| sigma | float | 0.5-5.0 | 1.5 | Edge Gaussian sigma - controls line thickness |
| tau | float | 0.9-1.0 | 0.99 | Weighting between Gaussians - lower detects more edges |
| phi | float | 0.5-10.0 | 2.0 | Threshold steepness - higher gives harder/crisper lines |

## Modulation Candidates

- **sigma**: line thickness pulses with audio - thin delicate lines in quiet passages, thick bold strokes on beats
- **tau**: edge sensitivity - lower values reveal more fine detail, higher values show only the strongest contours
- **phi**: hard vs soft edges - low phi gives soft charcoal shading, high phi gives sharp ink lines

### Interaction Patterns

- **Cascading threshold (tau vs phi)**: tau controls which edges are detected at all, phi controls how sharply they render. With tau near 1.0 (few edges), phi barely matters because there's little to threshold. As tau drops and more edges appear, phi determines whether they look like soft pencil or hard ink. Audio on tau "unlocks" edges that phi then sharpens.

## Notes

- Performance is O(kernel^2) per pixel, same as bilateral. With sigma 1.5, kernel is ~7x7 - very fast.
- sigma_r is derived as sigma * 1.6 internally (not exposed) - the 1.6 ratio is the standard Laplacian of Gaussian approximation and rarely needs changing.
- The reference computes on luminance only, which is correct - edge detection on per-channel RGB produces color fringing artifacts.
- Compositing: multiplying edge (0=dark, 1=bright) with input color preserves the original palette while adding ink contours. For "lines only" mode, the user can stack this after a Solid Color generator.
