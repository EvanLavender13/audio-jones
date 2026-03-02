// Based on "oil paint brush" by flockaroo (Florian Berger)
// https://www.shadertoy.com/view/MtKcDG
// License: CC BY-NC-SA 3.0 Unported
// Modified: adapted uniforms, removed COLORKEY_BG/CANVAS defines, hardcoded canvas strength

#version 330

// Oil Paint Stroke: Multi-scale grid distributes gradient-oriented brush strokes
// Iterates coarse-to-fine layers, compositing bristle-textured strokes along image edges

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;    // source image
uniform sampler2D texture1;    // shared noise texture (1024x1024 with mipmaps)
uniform vec2 resolution;       // render resolution (half-res in half-res pass)
uniform float brushSize;       // 0.5-3.0, default 1.0
uniform float strokeBend;      // -2.0-2.0, default -1.0
uniform float brushDetail;     // 0.01-0.5, default 0.1
uniform float srcContrast;     // 0.5-3.0, default 1.4
uniform float srcBright;       // 0.5-1.5, default 1.0
uniform int layers;            // 3-11, default 8

out vec4 finalColor;

vec4 getCol(vec2 pos, float lod) {
    vec2 uv = pos / resolution;
    return clamp(((textureLod(texture0, uv, lod) - 0.5) * srcContrast + 0.5 * srcBright), 0.0, 1.0);
}

vec3 getValCol(vec2 pos) {
    float texW = float(textureSize(texture0, 0).x);
    return getCol(pos, 1.5 + log2(texW / 600.0)).xyz;
}

vec4 getRand(int idx) {
    ivec2 texSize = textureSize(texture1, 0);
    idx = idx % (texSize.x * texSize.y);
    return texelFetch(texture1, ivec2(idx % texSize.x, idx / texSize.x), 0);
}

float compSignedMax(vec3 c) {
    vec3 a = abs(c);
    if (a.x > a.y && a.x > a.z) return c.x;
    if (a.y > a.x && a.y > a.z) return c.y;
    return c.z;
}

vec2 getGradMax(vec2 pos, float eps) {
    vec2 d = vec2(eps, 0.0);
    return vec2(
        compSignedMax(getValCol(pos + d.xy) - getValCol(pos - d.xy)),
        compSignedMax(getValCol(pos + d.yx) - getValCol(pos - d.yx))
    ) / eps / 2.0;
}

void main() {
    vec2 pos = fragTexCoord * resolution;
    vec2 noiseRes = vec2(textureSize(texture1, 0));
    float canv = 0.0;
    canv = max(canv, textureLod(texture1, pos * vec2(0.7, 0.03) / noiseRes, 0.0).x);
    canv = max(canv, textureLod(texture1, pos * vec2(0.03, 0.7) / noiseRes, 0.0).x);
    vec4 fragOut = vec4(vec3(0.93 + 0.07 * canv), 1.0);

    const float layerScaleFact = 0.85;
    float ls = layerScaleFact * layerScaleFact;
    int numGrid = int(float(0x8000) * min(pow(resolution.x / 1920.0, 0.5), 1.0) * (1.0 - ls));
    float aspect = resolution.x / resolution.y;
    int numX = int(sqrt(float(numGrid) * aspect) + 0.5);
    int numY = int(sqrt(float(numGrid) / aspect) + 0.5);
    int maxLayer = min(layers, int(log2(10.0 / float(numY)) / log2(layerScaleFact)));
    float gridW0 = resolution.x / float(numX);
    float resScale = sqrt(resolution.x / 600.0);

    // Fix 1: per-layer random index offset
    int pidx0 = 0;

    for (int layer = maxLayer; layer >= 0; layer--) {
        int numX2 = int(float(numX) * pow(layerScaleFact, float(layer)) + 0.5);
        int numY2 = int(float(numY) * pow(layerScaleFact, float(layer)) + 0.5);
        for (int ni = 0; ni < 9; ni++) {
            int nx = ni % 3 - 1;
            int ny = ni / 3 - 1;
            int n0 = int(dot(floor(pos / resolution * vec2(numX2, numY2)), vec2(1, numX2)));
            int pidx2 = n0 + numX2 * ny + nx;

            // Fix 1: unique random per layer
            int pidx = pidx0 + pidx2;

            vec2 brushPos = (vec2(pidx2 % numX2, pidx2 / numX2) + 0.5)
                            / vec2(numX2, numY2) * resolution;
            float gridW = resolution.x / float(numX2);

            // Fix 1: use pidx for all getRand calls
            vec4 rand = getRand(pidx);
            brushPos += gridW * (rand.xy - 0.5);
            brushPos.x += gridW * 0.5 * (float((pidx2 / numX2) % 2) - 0.5);

            // Gradient orientation
            vec2 g = getGradMax(brushPos, gridW * 1.0) * 0.5
                   + getGradMax(brushPos, gridW * 0.12) * 0.5
                   + 0.0003 * sin(pos / resolution * 20.0);
            float gl = length(g);
            vec2 n = normalize(g);
            vec2 t = n.yx * vec2(1, -1);

            // Stroke shape
            float stretch = sqrt(1.5 * pow(3.0, 1.0 / float(layer + 1)));
            float wh = (gridW - 0.6 * gridW0) * 1.2;
            float lh = wh;
            wh *= brushSize * (0.8 + 0.4 * rand.y) / stretch;
            lh *= brushSize * (0.8 + 0.4 * rand.z) * stretch;
            float wh0 = wh;

            // Fix 4: width pre-compensation for bend
            wh /= 1.0 - 0.25 * abs(strokeBend);

            // Fix 2: detail threshold for stroke culling
            wh = (gl * brushDetail < 0.003 / wh0 && wh0 < resolution.x * 0.02 && layer != maxLayer) ? 0.0 : wh;

            vec2 uv = vec2(dot(pos - brushPos, n), dot(pos - brushPos, t))
                       / vec2(wh, lh) * 0.5;

            // Stroke bending
            uv.x -= 0.125 * strokeBend;
            uv.x += uv.y * uv.y * strokeBend;
            uv.x /= 1.0 - 0.25 * abs(strokeBend);
            uv += 0.5;

            // Alpha mask
            float s = 1.0;
            s *= uv.x * (1.0 - uv.x) * 6.0;
            s *= uv.y * (1.0 - uv.y) * 6.0;
            float s0 = s;
            s = clamp((s - 0.5) * 2.0, 0.0, 1.0);

            // Fix 3: copy uv to uv0 and flip y
            vec2 uv0 = uv;

            // Fix 8: brush hair noise at mip 1
            float pat = textureLod(texture1, uv * 1.5 * resScale * vec2(0.06, 0.006), 1.0).x
                      + textureLod(texture1, uv * 3.0 * resScale * vec2(0.06, 0.006), 1.0).x;

            s0 = s;
            s *= 0.7 * pat;

            // Fix 3: y-flip before smask and thickness falloff
            uv0.y = 1.0 - uv0.y;
            float smask = clamp(
                max(cos(uv0.x * 3.14159265 * 2.0 + 1.5 * (rand.x - 0.5)),
                    (1.5 * exp(-uv0.y * uv0.y / 0.0225) + 0.2) * (1.0 - uv0.y)) + 0.1,
                0.0, 1.0);
            s += s0 * smask;
            s -= 0.5 * uv0.y;
            s = clamp(s, 0.0, 1.0);

            // Fix 6: color sampling at mip 1
            vec3 strokeColor = getCol(brushPos, 1.0).xyz
                             * mix(s * 0.13 + 0.87, 1.0, smask);

            // Compositing
            float mask = s * step(-0.5, -abs(uv0.x - 0.5)) * step(-0.5, -abs(uv0.y - 0.5));
            fragOut = mix(fragOut, vec4(strokeColor, 1.0), mask);
        }

        // Fix 1: accumulate offset for next layer
        pidx0 += numX2 * numY2;
    }

    finalColor = fragOut;
}
