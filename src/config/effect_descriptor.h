#ifndef EFFECT_DESCRIPTOR_H
#define EFFECT_DESCRIPTOR_H

#include "effect_config.h"
#include <raylib.h>
#include <stddef.h>
#include <stdint.h>

// Forward declarations for function pointer types
typedef struct PostEffect PostEffect;

// BlendCompositor must be a complete type for
// REGISTER_GENERATOR/REGISTER_SIM_BOOST macros which dereference
// pe->blendCompositor->shader
#include "render/blend_compositor.h"

// Callback for shader uniform binding (moved here from shader_setup.h)
typedef void (*RenderPipelineShaderSetupFn)(PostEffect *pe);

// Flag bitmask for effect routing and capabilities
#define EFFECT_FLAG_NONE 0
#define EFFECT_FLAG_BLEND 1
#define EFFECT_FLAG_HALF_RES 2
#define EFFECT_FLAG_SIM_BOOST 4
#define EFFECT_FLAG_NEEDS_RESIZE 8

struct EffectDescriptor {
  // Metadata
  TransformEffectType type;
  const char *name;
  const char *categoryBadge;
  int categorySectionIndex;
  size_t enabledOffset;
  uint8_t flags;

  // Lifecycle function pointers (NULL when not applicable)
  bool (*init)(PostEffect *pe, int w, int h);
  void (*uninit)(PostEffect *pe);
  void (*resize)(PostEffect *pe, int w, int h);
  void (*registerParams)(EffectConfig *cfg);

  // Dispatch (replaces GetTransformEffect switch)
  Shader *(*getShader)(PostEffect *pe);
  RenderPipelineShaderSetupFn setup;
};

// Effect descriptor table indexed by TransformEffectType
extern EffectDescriptor EFFECT_DESCRIPTORS[TRANSFORM_EFFECT_COUNT];

// Register an effect descriptor into the table. Returns true.
bool EffectDescriptorRegister(TransformEffectType type,
                              const EffectDescriptor &desc);

// Category badge and section index pair
struct EffectCategory {
  const char *badge;
  int sectionIndex;
};

// Returns display name for the given effect type
const char *EffectDescriptorName(TransformEffectType type);

// Returns category badge string and section color index
EffectCategory EffectDescriptorCategory(TransformEffectType type);

// Reads the enabled bool at the descriptor's enabledOffset within EffectConfig
bool IsDescriptorEnabled(const EffectConfig *e, TransformEffectType type);

// Convenience wrappers preserving the original API names
inline const char *TransformEffectName(TransformEffectType type) {
  return EffectDescriptorName(type);
}

inline bool IsTransformEnabled(const EffectConfig *e,
                               TransformEffectType type) {
  return IsDescriptorEnabled(e, type);
}

// ---------------------------------------------------------------------------
// Self-registration macros
//
// Each macro goes at the bottom of an effect .cpp file. It generates static
// wrapper functions that adapt the effect's own Init/Uninit/Resize/
// RegisterParams/GetShader signatures to the uniform EffectDescriptor
// function-pointer signatures, then registers the descriptor at static-init
// time via a file-local bool.
//
// The expansion site must already #include "render/post_effect.h" so that
// PostEffect and BlendCompositor are complete types.
// ---------------------------------------------------------------------------

// clang-format off

// --- REGISTER_EFFECT: Init(Effect*) ---
#define REGISTER_EFFECT(Type, Name, field, displayName, badge, section,        \
                        flags, SetupFn, ResizeFn)                              \
  void SetupFn(PostEffect *);                                                  \
  static bool Init_##field(PostEffect *pe, int, int) {                         \
    return Name##EffectInit(&pe->field);                                        \
  }                                                                            \
  static void Uninit_##field(PostEffect *pe) {                                 \
    Name##EffectUninit(&pe->field);                                            \
  }                                                                            \
  static void Register_##field(EffectConfig *cfg) {                            \
    Name##RegisterParams(&cfg->field);                                         \
  }                                                                            \
  static Shader *GetShader_##field(PostEffect *pe) {                           \
    return &pe->field.shader;                                                  \
  }                                                                            \
  static bool reg_##field = EffectDescriptorRegister(                          \
      Type,                                                                    \
      EffectDescriptor{Type, displayName, badge, section,                      \
       offsetof(EffectConfig, field.enabled), (uint8_t)(flags),                \
       Init_##field, Uninit_##field, ResizeFn, Register_##field,               \
       GetShader_##field, SetupFn});

// --- REGISTER_EFFECT_CFG: Init(Effect*, Config*) ---
#define REGISTER_EFFECT_CFG(Type, Name, field, displayName, badge, section,    \
                            flags, SetupFn, ResizeFn)                          \
  void SetupFn(PostEffect *);                                                  \
  static bool Init_##field(PostEffect *pe, int, int) {                         \
    return Name##EffectInit(&pe->field, &pe->effects.field);                   \
  }                                                                            \
  static void Uninit_##field(PostEffect *pe) {                                 \
    Name##EffectUninit(&pe->field);                                            \
  }                                                                            \
  static void Register_##field(EffectConfig *cfg) {                            \
    Name##RegisterParams(&cfg->field);                                         \
  }                                                                            \
  static Shader *GetShader_##field(PostEffect *pe) {                           \
    return &pe->field.shader;                                                  \
  }                                                                            \
  static bool reg_##field = EffectDescriptorRegister(                          \
      Type,                                                                    \
      EffectDescriptor{Type, displayName, badge, section,                      \
       offsetof(EffectConfig, field.enabled), (uint8_t)(flags),                \
       Init_##field, Uninit_##field, ResizeFn, Register_##field,               \
       GetShader_##field, SetupFn});

// --- REGISTER_GENERATOR: CFG init, GEN badge, section 10, BLEND flag ---
// GetShader returns &pe->blendCompositor->shader
#define REGISTER_GENERATOR(Type, Name, field, displayName, SetupFn)            \
  void SetupFn(PostEffect *);                                                  \
  static bool Init_##field(PostEffect *pe, int, int) {                         \
    return Name##EffectInit(&pe->field, &pe->effects.field);                   \
  }                                                                            \
  static void Uninit_##field(PostEffect *pe) {                                 \
    Name##EffectUninit(&pe->field);                                            \
  }                                                                            \
  static void Register_##field(EffectConfig *cfg) {                            \
    Name##RegisterParams(&cfg->field);                                         \
  }                                                                            \
  static Shader *GetShader_##field(PostEffect *pe) {                           \
    return &pe->blendCompositor->shader;                                       \
  }                                                                            \
  static bool reg_##field = EffectDescriptorRegister(                          \
      Type,                                                                    \
      EffectDescriptor{Type, displayName, "GEN", 10,                           \
       offsetof(EffectConfig, field.enabled), EFFECT_FLAG_BLEND,               \
       Init_##field, Uninit_##field, NULL, Register_##field,                   \
       GetShader_##field, SetupFn});

// --- REGISTER_GENERATOR_FULL: FULL init (cfg + sized), with resize ---
#define REGISTER_GENERATOR_FULL(Type, Name, field, displayName, SetupFn)       \
  void SetupFn(PostEffect *);                                                  \
  static bool Init_##field(PostEffect *pe, int w, int h) {                     \
    return Name##EffectInit(&pe->field, &pe->effects.field, w, h);             \
  }                                                                            \
  static void Uninit_##field(PostEffect *pe) {                                 \
    Name##EffectUninit(&pe->field);                                            \
  }                                                                            \
  static void Resize_##field(PostEffect *pe, int w, int h) {                   \
    Name##EffectResize(&pe->field, w, h);                                      \
  }                                                                            \
  static void Register_##field(EffectConfig *cfg) {                            \
    Name##RegisterParams(&cfg->field);                                         \
  }                                                                            \
  static Shader *GetShader_##field(PostEffect *pe) {                           \
    return &pe->blendCompositor->shader;                                       \
  }                                                                            \
  static bool reg_##field = EffectDescriptorRegister(                          \
      Type,                                                                    \
      EffectDescriptor{Type, displayName, "GEN", 10,                           \
       offsetof(EffectConfig, field.enabled),                                  \
       (uint8_t)(EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE),                \
       Init_##field, Uninit_##field, Resize_##field, Register_##field,         \
       GetShader_##field, SetupFn});

// --- REGISTER_SIM_BOOST: no init/uninit/resize, blend compositor shader ---
#define REGISTER_SIM_BOOST(Type, field, displayName, SetupFn, RegisterFn)      \
  void SetupFn(PostEffect *);                                                  \
  static void Register_##field(EffectConfig *cfg) {                            \
    RegisterFn(&cfg->field);                                                   \
  }                                                                            \
  static Shader *GetShader_##field(PostEffect *pe) {                           \
    return &pe->blendCompositor->shader;                                       \
  }                                                                            \
  static bool reg_##field = EffectDescriptorRegister(                          \
      Type,                                                                    \
      EffectDescriptor{Type, displayName, "SIM", 9,                            \
       offsetof(EffectConfig, field.enabled), EFFECT_FLAG_SIM_BOOST,           \
       NULL, NULL, NULL, Register_##field,                                     \
       GetShader_##field, SetupFn});

// clang-format on

#endif // EFFECT_DESCRIPTOR_H
