// Inspired by "2D Perspective" by Whirl
// https://godotshaders.com/shader/2d-perspective/
// Adapted for AudioJones: GLSL 330, resolution uniform, autoZoom as int toggle

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float pitch;    // radians
uniform float yaw;      // radians
uniform float roll;     // radians
uniform float fov;      // degrees
uniform int autoZoom;   // 1 = on, 0 = off

void main() {
    float aspect = resolution.x / resolution.y;

    // 1. Center and aspect-correct
    vec2 p = (fragTexCoord - 0.5) * vec2(aspect, 1.0);

    // 2. Lift to 3D ray — z encodes FOV
    //    Low FOV → large z → mild foreshortening
    //    High FOV → small z → dramatic perspective
    float zFov = 0.5 / tan(fov * 3.14159265 / 360.0);
    vec3 ray = vec3(p, zFov);

    // 3-5. Combined YXZ rotation (yaw, then pitch, then roll)
    //
    // Yaw (Y-axis) × Pitch (X-axis) matrix:
    //   R[0] = ( cos_y,              sin_y * sin_p,  sin_y * cos_p )
    //   R[1] = ( 0,                  cos_p,         -sin_p         )
    //   R[2] = (-sin_y,              cos_y * sin_p,  cos_y * cos_p )
    //
    float cp = cos(pitch), sp = sin(pitch);
    float cy = cos(yaw),   sy = sin(yaw);

    vec3 rotated = vec3(
        cy * ray.x + sy * sp * ray.y + sy * cp * ray.z,
                              cp * ray.y -      sp * ray.z,
       -sy * ray.x + cy * sp * ray.y + cy * cp * ray.z
    );

    // Roll (Z-axis) applied to rotated.xy
    float cr = cos(roll), sr = sin(roll);
    rotated.xy = vec2(
        cr * rotated.x - sr * rotated.y,
        sr * rotated.x + cr * rotated.y
    );

    // Back-face cull: if z <= 0 the plane faces away
    if (rotated.z <= 0.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // 6. Perspective divide
    vec2 tilted = rotated.xy / rotated.z;

    // 7. Auto-zoom: project all 4 corners through the same transform,
    //    find bounding extent, scale tilted so warped image fills the screen
    if (autoZoom == 1) {
        float zoom = 0.0;
        for (int i = 0; i < 4; i++) {
            vec2 corner = vec2(
                (i < 2 ? -1.0 : 1.0) * aspect * 0.5,
                (i == 0 || i == 3 ? -0.5 : 0.5)
            );
            vec3 cr3 = vec3(corner, zFov);
            vec3 rc = vec3(
                cy * cr3.x + sy * sp * cr3.y + sy * cp * cr3.z,
                                     cp * cr3.y -      sp * cr3.z,
               -sy * cr3.x + cy * sp * cr3.y + cy * cp * cr3.z
            );
            rc.xy = vec2(cr * rc.x - sr * rc.y, sr * rc.x + cr * rc.y);
            if (rc.z > 0.0) {
                vec2 proj = rc.xy / rc.z;
                zoom = max(zoom, max(abs(proj.x) / (aspect * 0.5),
                                     abs(proj.y) / 0.5));
            }
        }
        if (zoom > 0.0) tilted /= zoom;
    }

    // 8. Un-correct aspect and map back to texture UV
    vec2 uv = tilted / vec2(aspect, 1.0) + 0.5;

    // 9. Bounds check — output black outside [0,1]
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    finalColor = texture(texture0, uv);
}
