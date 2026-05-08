// Based on "Voxel Pillars webcam Y28" by Yusef28
// https://www.shadertoy.com/view/s3XGWH
// License: CC BY-NC-SA 3.0 Unported
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float density;
uniform float pitch;
uniform float heightScale;
uniform float cornerRadius;


vec2 fieldSize() {
    float aspect = resolution.x / resolution.y;
    return vec2(density, density / aspect);
}

vec2 cellUVForId(vec2 id) {
    vec2 fs = fieldSize();
    return (id + fs * 0.5 + 0.5) / fs;
}

float getGrey(vec3 c) {
    return c.x * 0.299 + c.y * 0.587 + c.z * 0.114;
}

float roundBox(vec3 p, vec3 b, float r) {
    return length(max(abs(p) - b, 0.0)) - r;
}

float tile(vec3 p) {
    const float N = 2.0;
    vec2 id = floor(p.xz);
    p.xz = fract(p.xz) - 0.5;
    float d = 100.0;
    float fill = max(0.57 - cornerRadius, 0.0);
    for (float i = -N; i <= N; i++) {
        for (float j = -N; j <= N; j++) {
            vec2 n = vec2(i, j);
            vec2 uv = cellUVForId(id + n);
            if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
                continue;
            }
            float h = max(getGrey(textureLod(texture0, uv, 2.0).rgb) * heightScale, 0.05);
            d = min(d, roundBox(p - vec3(n.x, -3.0, n.y),
                                vec3(fill, h, fill),
                                cornerRadius));
        }
    }
    return d;
}

float map(vec3 p) {
    return tile(p);
}

float trace(vec3 ro, vec3 rd, float far) {
    float t = 0.0;
    for (int i = 0; i < 128; i++) {
        float dist = map(ro + rd * t);
        if (dist < 0.0001 || t > far) break;
        t += dist * 0.85;
    }
    return t;
}

vec3 normal(vec3 p) {
    mat3 k = mat3(p, p, p) - mat3(0.01);
    return normalize(map(p) - vec3(map(k[0]), map(k[1]), map(k[2])));
}

float calculateAO(vec3 pos, vec3 nor) {
    float sca = 2.0, occ = 0.0;
    for (int i = 0; i < 5; i++) {
        float hr = 0.01 + float(i) * 0.5 / 4.0;
        float dd = map(nor * hr + pos);
        occ += (hr - dd) * sca;
        sca *= 0.7;
    }
    return clamp(1.0 - occ, 0.0, 1.0);
}

vec3 lighting(vec3 sp, vec3 sn, vec3 lp, vec3 rd, vec3 baseColor) {
    vec3 lv = lp - sp;
    vec3 ldir = normalize(lv);
    float diff = dot(ldir, sn);
    float spec = pow(max(dot(reflect(-ldir, sn), -rd), 0.0), 10.0);
    float ao = calculateAO(sp, sn);
    vec3 hotSpec = vec3(1.0);
    vec3 color = diff * baseColor + spec * hotSpec;
    return clamp(color * ao, 0.0, 1.0);
}

void main() {
    float aspect = resolution.x / resolution.y;
    vec2 uv = (fragTexCoord - 0.5) * vec2(aspect, 1.0);

    // -0.25 z offset transcribed from reference; keeps fwd.z nonzero at pitch = HALF_PI.
    float R = max(density / 3.0 - 3.5, 5.0);
    vec3 lk = vec3(0.0);
    vec3 ro = vec3(0.0, R * sin(pitch), -R * cos(pitch) - 0.25);
    vec3 lp = ro + vec3(0.0, 3.75, 10.0);

    float FOV = 1.57;
    vec3 fwd = normalize(lk - ro);
    vec3 rgt = normalize(vec3(fwd.z, 0.0, -fwd.x));
    vec3 up = cross(fwd, rgt);
    vec3 rd = normalize(fwd + FOV * uv.x * rgt + FOV * uv.y * up);

    float far = max(60.0, density * 2.0);
    float t = trace(ro, rd, far);

    if (t > far) {
        finalColor = vec4(texture(texture0, fragTexCoord).rgb, 1.0);
        return;
    }

    vec3 sp = ro + rd * t;
    vec3 sn = normal(sp);

    vec2 hitUV = cellUVForId(floor(sp.xz));
    vec3 baseColor = texture(texture0, hitUV).rgb;

    vec3 color = lighting(sp, sn, lp, rd, baseColor);

    float vig = 1.0 - smoothstep(1.0, 3.5, length(uv));
    color *= mix(0.8, 1.0, vig);

    finalColor = vec4(color, 1.0);
}
