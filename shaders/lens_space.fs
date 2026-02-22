#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform vec2 center;
uniform vec2 sphereOffset;
uniform float p;
uniform float q;
uniform float sphereRadius;
uniform float boundaryRadius;
uniform float rotAngle;
uniform float maxReflections;
uniform float dimming;
uniform float zoom;
uniform float projScale;

#define PI 3.14159265

mat2 Rot(float th) {
    float c = cos(th);
    float s = sin(th);
    return mat2(vec2(c, s), vec2(-s, c));
}

void main() {
    vec2 uv = (fragTexCoord - center) * resolution / resolution.y;

    vec3 ro = vec3(0.0, 0.0, -0.5);
    vec3 rd = vec3(uv / zoom, 1.0);
    rd.xz *= Rot(rotAngle);

    vec3 sc = vec3(sphereOffset, 0.5);

    float t = 0.0;
    vec3 col = vec3(0.0);
    float refs = 0.0;
    bool hitSphere = false;

    for (int i = 0; i < 300; i++) {
        vec3 pos = ro + t * rd;

        float d = length(pos - sc) - sphereRadius;
        float e = boundaryRadius - length(pos);

        if (d < 0.001) {
            vec3 n = normalize(pos - sc);
            vec3 r = reflect(rd, n);
            col = texture(texture0, 0.5 + r.xy / max(abs(r.z), 0.001) * projScale).rgb;
            hitSphere = true;
            break;
        }

        if (abs(e) < 0.001) {
            mat2 R = Rot(q * PI / p);
            pos.xz = R * pos.xz;
            rd.xz = R * rd.xz;

            vec3 n = normalize(-pos);
            rd = reflect(rd, n);

            ro = pos + 0.01 * n;
            t = 0.0;

            if (refs >= maxReflections) break;
            refs += 1.0;
        }

        t += min(e * 0.8, d * 0.4);
    }

    if (!hitSphere) {
        col = texture(texture0, 0.5 + rd.xy / max(abs(rd.z), 0.001) * projScale).rgb;
    }

    col *= 1.0 - refs * dimming;

    finalColor = vec4(col, 1.0);
}
