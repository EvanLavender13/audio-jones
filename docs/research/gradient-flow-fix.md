# Gradient Flow Fix

The current implementation produces pixelated noise because Sobel gradients are computed per-pixel without spatial smoothing. The fix: replace raw Sobel with a structure tensor that averages gradient information over a window.

## Problem

Sobel operates on a 3x3 neighborhood. After one displacement iteration, the new position has a completely different local gradient. These direction changes accumulate into noise.

## Fix: Structure Tensor

The structure tensor accumulates gradient products (Ix², Iy², IxIy) over a window, then extracts the dominant orientation via eigendecomposition. This produces spatially coherent flow directions.

### Algorithm

1. For each pixel in a window around the sample point:
   - Compute gradient Ix, Iy via central difference
   - Accumulate Ix², Iy², IxIy
2. Average the accumulated products
3. Extract orientation: α = 0.5 × atan2(2×J12, J22 - J11)
4. Flow direction: (cos(α), sin(α)) for tangent, rotate by flowAngle

### Shader Code

```glsl
vec2 computeFlowDirection(vec2 uv, vec2 texel, int halfSize) {
    float J11 = 0.0, J22 = 0.0, J12 = 0.0;

    for (int y = -halfSize; y <= halfSize; y++) {
        for (int x = -halfSize; x <= halfSize; x++) {
            vec2 sampleUV = uv + vec2(float(x), float(y)) * texel;

            float l = luminance(texture(texture0, sampleUV + vec2(-texel.x, 0.0)).rgb);
            float r = luminance(texture(texture0, sampleUV + vec2( texel.x, 0.0)).rgb);
            float t = luminance(texture(texture0, sampleUV + vec2(0.0, -texel.y)).rgb);
            float b = luminance(texture(texture0, sampleUV + vec2(0.0,  texel.y)).rgb);

            float Ix = (r - l) * 0.5;
            float Iy = (b - t) * 0.5;

            J11 += Ix * Ix;
            J22 += Iy * Iy;
            J12 += Ix * Iy;
        }
    }

    // Orientation from structure tensor
    float angle = 0.5 * atan(2.0 * J12, J22 - J11);
    vec2 tangent = vec2(cos(angle), sin(angle));

    // Rotate by flowAngle (0 = along edges, PI/2 = across edges)
    float s = sin(flowAngle);
    float c = cos(flowAngle);
    return vec2(c * tangent.x - s * tangent.y, s * tangent.x + c * tangent.y);
}
```

### New Parameter

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| smoothRadius | int | 1-4 | 2 | Window half-size (2 = 5×5 window) |

### Performance

Window size 5×5 with 8 iterations: ~1000 texture samples per pixel. Acceptable for a transform effect. Reduce iterations to 4 if needed—the smoother field requires fewer iterations for clean results.

## References

- [OpenCV: Gradient Structure Tensor](https://docs.opencv.org/4.x/d4/d70/tutorial_anisotropic_image_segmentation_by_a_gst.html)
- [CMU Structure Tensor Tutorial](https://www.cs.cmu.edu/~sarsen/structureTensorTutorial/)
