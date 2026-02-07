#ifndef CONSTANTS_H
#define CONSTANTS_H

// Pure math and bounds constants — no UI or library dependencies

#define PI_F 3.14159265f
#define TWO_PI_F 6.28318530718f

#define RAD_TO_DEG 57.2957795131f
#define DEG_TO_RAD 0.01745329251f

// Rotation bounds: speeds use ±180 deg/s max, offsets use ±180 deg
#define ROTATION_SPEED_MAX PI_F  // 180 deg/s in radians
#define ROTATION_OFFSET_MAX PI_F // 180 deg in radians

// LFO rate bounds
#define LFO_RATE_MIN 0.001f // Hz
#define LFO_RATE_MAX 5.0f   // Hz

#endif // CONSTANTS_H
