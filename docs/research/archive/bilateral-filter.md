# Bilateral Filter

Edge-preserving smoothing that averages nearby pixels weighted by both spatial distance and color similarity, producing a porcelain/CGI look where flat regions blur smooth while hard edges stay crisp.

## Classification

- **Category**: TRANSFORMS > Painterly
- **Pipeline Position**: Same slot as Kuwahara (transform chain, user-ordered)

## References

- [tranvansang/bilateral-filter](https://github.com/tranvansang/bilateral-filter) - Clean GLSL implementation with both sigmas as uniforms
- [genekogan/Processing-Shader-Examples](https://github.com/genekogan/Processing-Shader-Examples/blob/master/TextureShaders/data/bilateral_filter.glsl) - Widely used implementation (originally by mrharicot)
- [LYGIA Shader Library](https://lygia.xyz/filter/bilateral) - Library-quality bilateral with customizable types

## Reference Code

From tranvansang/bilateral-filter (adapted to GLSL 330 conventions):

```glsl
#version 450
uniform sampler2D texture;
in vec2 v_texcoord;

uniform float sigmaS;
uniform float sigmaL;

#define EPS 1e-5

out vec4 FragColor;

float lum(in vec4 color) {
    return length(color.xyz);
}

void main() {
    float sigS = max(sigmaS, EPS);
    float sigL = max(sigmaL, EPS);

    float facS = -1./(2.*sigS*sigS);
    float facL = -1./(2.*sigL*sigL);

    float sumW = 0.;
    vec4  sumC = vec4(0.);
    float halfSize = sigS * 2;
    ivec2 textureSize2 = textureSize(texture, 0);

    float l = lum(texture2D(texture, v_texcoord));

    for (float i = -halfSize; i <= halfSize; i++) {
        for (float j = -halfSize; j <= halfSize; j++) {
            vec2 pos = vec2(i, j);
            vec4 offsetColor = texture2D(texture, v_texcoord + pos / textureSize2);

            float distS = length(pos);
            float distL = lum(offsetColor) - l;

            float wS = exp(facS * float(distS*distS));
            float wL = exp(facL * float(distL*distL));
            float w = wS * wL;

            sumW += w;
            sumC += offsetColor * w;
        }
    }

    FragColor = sumC / sumW;
}
```

From genekogan (mrharicot origin), alternative normpdf approach:

```glsl
#define BSIGMA 0.1
#define MSIZE 10

uniform vec2      resolution;
uniform sampler2D texture;
uniform float sigma;

float normpdf(in float x, in float sigma) {
    return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

float normpdf3(in vec3 v, in float sigma) {
    return 0.39894*exp(-0.5*dot(v,v)/(sigma*sigma))/sigma;
}

void main(void) {
    vec2 t = gl_FragCoord.xy / resolution.xy;
    vec3 c = texture2D(texture, t).rgb;

    const int kSize = (MSIZE-1)/2;
    float kernel[MSIZE];
    vec3 final_colour = vec3(0.0);

    float Z = 0.0;
    for (int j = 0; j <= kSize; ++j) {
        kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), sigma);
    }

    vec3 cc;
    float factor;
    float bZ = 1.0/normpdf(0.0, BSIGMA);

    for (int i=-kSize; i <= kSize; ++i) {
        for (int j=-kSize; j <= kSize; ++j) {
            vec2 tt = (gl_FragCoord.xy+vec2(float(i),float(j))) / resolution.xy;
            cc = texture2D(texture, tt).rgb;
            factor = normpdf3(cc-c, BSIGMA)*bZ*kernel[kSize+j]*kernel[kSize+i];
            Z += factor;
            final_colour += factor*cc;
        }
    }

    gl_FragColor = vec4(final_colour/Z, 1.0);
}
```

## Algorithm

Adapt the tranvansang implementation to this codebase's conventions:

- Keep the dual-Gaussian weighting verbatim: `exp(facS * distS^2)` for spatial, `exp(facL * distL^2)` for range
- Keep `length(color.xyz)` as the luminance function (simple, fast)
- Replace `#version 450` with `#version 330`
- Replace `texture2D()` with `texture()`
- Replace `textureSize(texture, 0)` with `resolution` uniform (consistent with other effects)
- Replace `v_texcoord` with `fragTexCoord`
- Pixel offset: `pos / resolution` instead of `pos / textureSize2`
- Use `int halfSize` with dynamic loop bounds (GLSL 330 supports this)
- Cap kernel radius to prevent GPU stalls: `min(halfSize, 12)`

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| spatialSigma | float | 1.0-10.0 | 4.0 | Blur radius in pixels - higher smooths larger regions |
| rangeSigma | float | 0.01-0.5 | 0.1 | Edge sensitivity - lower preserves more edges |

## Modulation Candidates

- **spatialSigma**: widens/narrows the smoothing area, making the image alternate between detailed and plastic
- **rangeSigma**: controls how aggressively edges are preserved - low values create sharp flat-vs-detailed boundaries, high values let everything blur together

### Interaction Patterns

- **Competing forces (spatialSigma vs rangeSigma)**: High spatial with low range gives the signature porcelain look (strong smoothing that respects edges). High spatial with high range collapses into plain blur. The tension between "how far to reach" and "how picky about color similarity" defines the character of the output. Audio driving these in opposite directions creates a push-pull between detail preservation and smoothing.

## Notes

- Performance is O(kernel^2) per pixel, same cost class as Kuwahara. Kernel radius of 4-8 is practical for real-time.
- Unlike Kuwahara (which picks the region with lowest variance), bilateral weights ALL neighbors by similarity. This gives smoother gradients instead of flat plateaus.
- Could add an `iterations` int param (1-3) for multi-pass if single-pass isn't strong enough, but start without it.
