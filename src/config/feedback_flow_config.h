#ifndef FEEDBACK_FLOW_CONFIG_H
#define FEEDBACK_FLOW_CONFIG_H

// Feedback Flow: UV displacement driven by luminance gradients
// Displaces perpendicular or parallel to edges based on flowAngle, creating
// organic flowing distortion
struct FeedbackFlowConfig {
  float strength =
      0.0f; // Displacement magnitude in pixels (0.0-20.0, 0 = disabled)
  float flowAngle = 0.0f; // Direction relative to gradient in radians (0 =
                          // perpendicular, pi/2 = parallel)
  float scale = 1.0f; // Sampling distance for gradient computation (1.0-5.0)
  float threshold =
      0.001f; // Minimum gradient magnitude to trigger flow (0.0-0.1)
};

#define FEEDBACK_FLOW_CONFIG_FIELDS strength, flowAngle, scale, threshold

#endif // FEEDBACK_FLOW_CONFIG_H
