#include "effect_descriptor.h"
#include <stddef.h>

// Zero-initialized table — slots populated by REGISTER_EFFECT* macros at
// static-init time
EffectDescriptor EFFECT_DESCRIPTORS[TRANSFORM_EFFECT_COUNT] = {};

bool EffectDescriptorRegister(TransformEffectType type,
                              const EffectDescriptor &desc) {
  EFFECT_DESCRIPTORS[type] = desc;
  return true;
}

const char *EffectDescriptorName(TransformEffectType type) {
  if (type >= 0 && type < TRANSFORM_EFFECT_COUNT) {
    return EFFECT_DESCRIPTORS[type].name;
  }
  return "Unknown";
}

EffectCategory EffectDescriptorCategory(TransformEffectType type) {
  if (type >= 0 && type < TRANSFORM_EFFECT_COUNT) {
    return {EFFECT_DESCRIPTORS[type].categoryBadge,
            EFFECT_DESCRIPTORS[type].categorySectionIndex};
  }
  return {"???", 0};
}

bool IsDescriptorEnabled(const EffectConfig *e, TransformEffectType type) {
  if (e == NULL || type < 0 || type >= TRANSFORM_EFFECT_COUNT) {
    return false;
  }
  const char *base = (const char *)e;
  return *(const bool *)(base + EFFECT_DESCRIPTORS[type].enabledOffset);
}
