#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec2 resolution;
uniform float brushSize;
uniform float brushDetail;
uniform float strokeBend;
uniform int quality;
uniform int layers;

out vec4 finalColor;

vec4 getRand(int idx) {
    ivec2 texSize = textureSize(texture1, 0);
    ivec2 coord = ivec2(idx % texSize.x, idx / texSize.x) % texSize;
    return texelFetch(texture1, coord, 0);
}

vec3 getVal(vec2 pos, float lod) {
    return textureLod(texture0, pos / resolution, lod).rgb;
}

float compsignedmax(vec3 c) {
    vec3 a = abs(c);
    if (a.x > a.y && a.x > a.z) return c.x;
    if (a.y > a.x && a.y > a.z) return c.y;
    return c.z;
}

vec2 getGradMax(vec2 pos, float eps) {
    float lod = log2(2.0 * eps);
    vec2 d = vec2(eps, 0.0);
    return vec2(
        compsignedmax(getVal(pos + d.xy, lod) - getVal(pos - d.xy, lod)),
        compsignedmax(getVal(pos + d.yx, lod) - getVal(pos - d.yx, lod))
    ) / eps / 2.0;
}

void main() {
    vec2 pos = fragTexCoord * resolution;
    vec4 fragOut = texture(texture0, fragTexCoord);

    float layerScaleFact = float(quality) / 100.0;
    float ls = layerScaleFact * layerScaleFact;
    int NumGrid = int(float(0x8000) * min(pow(resolution.x / 1920.0, 0.5), 1.0) * (1.0 - ls));
    float aspect = resolution.x / resolution.y;
    int NumX = int(sqrt(float(NumGrid) * aspect) + 0.5);
    int NumY = int(sqrt(float(NumGrid) / aspect) + 0.5);
    int maxLayer = min(layers, int(log2(10.0 / float(NumY)) / log2(layerScaleFact)));
    float gridW0 = resolution.x / float(NumX);

    for (int layer = maxLayer; layer >= 0; layer--) {
        int NumX2 = int(float(NumX) * pow(layerScaleFact, float(layer)) + 0.5);
        int NumY2 = int(float(NumY) * pow(layerScaleFact, float(layer)) + 0.5);
        for (int ni = 0; ni < 9; ni++) {
            int nx = ni % 3 - 1;
            int ny = ni / 3 - 1;
            int n0 = int(dot(floor(pos / resolution * vec2(NumX2, NumY2)), vec2(1, NumX2)));
            int pidx2 = n0 + NumX2 * ny + nx;
            vec2 brushPos = (vec2(pidx2 % NumX2, pidx2 / NumX2) + 0.5)
                            / vec2(NumX2, NumY2) * resolution;
            float gridW = resolution.x / float(NumX2);
            vec4 rand = getRand(pidx2);
            brushPos += gridW * (rand.xy - 0.5);
            brushPos.x += gridW * 0.5 * (float((pidx2 / NumX2) % 2) - 0.5);

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
            float wh0 = wh;
            wh *= brushSize * (0.8 + 0.4 * rand.y) / stretch;
            lh *= brushSize * (0.8 + 0.4 * rand.z) * stretch;

            vec2 uv = vec2(dot(pos - brushPos, n), dot(pos - brushPos, t))
                       / vec2(wh, lh) * 0.5 + 0.5;

            // Suppress strokes in flat areas
            wh = (gl * brushDetail < 0.003 / wh0 && wh0 < resolution.x * 0.02 && layer != maxLayer)
                 ? 0.0 : wh;

            // Stroke bending
            uv.x -= 0.125 * strokeBend;
            uv.x += uv.y * uv.y * strokeBend;
            uv.x /= 1.0 - 0.25 * abs(strokeBend);

            // Alpha mask and brush hair texture
            float s = 1.0;
            s *= uv.x * (1.0 - uv.x) * 6.0;
            s *= uv.y * (1.0 - uv.y) * 6.0;
            float s0 = clamp((s - 0.5) * 2.0, 0.0, 1.0);

            float pat = texture(texture1, uv * 1.5 * sqrt(resolution.x / 600.0) * vec2(0.06, 0.006)).x
                      + texture(texture1, uv * 3.0 * sqrt(resolution.x / 600.0) * vec2(0.06, 0.006)).x;

            float alpha = s0 * 0.7 * pat;

            float smask = clamp(
                max(cos(uv.x * 3.14159265 * 2.0 + 1.5 * (rand.x - 0.5)),
                    (1.5 * exp(-uv.y * uv.y / 0.0225) + 0.2) * (1.0 - uv.y)) + 0.1,
                0.0, 1.0);
            alpha += s0 * smask;
            alpha -= 0.5 * uv.y;
            alpha = clamp(alpha, 0.0, 1.0);

            // Color sampling and compositing
            vec3 strokeColor = texture(texture0, brushPos / resolution).rgb
                             * mix(alpha * 0.13 + 0.87, 1.0, smask);

            float mask = alpha * step(-0.5, -abs(uv.x - 0.5)) * step(-0.5, -abs(uv.y - 0.5));

            fragOut = mix(fragOut, vec4(strokeColor, 1.0), mask);
        }
    }

    finalColor = fragOut;
}
