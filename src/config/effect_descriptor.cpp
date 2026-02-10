#include "effect_descriptor.h"
#include <stddef.h>
#include <stdint.h>

const EffectDescriptor *EffectDescriptorGet(TransformEffectType type) {
  if (type >= 0 && type < TRANSFORM_EFFECT_COUNT) {
    return &EFFECT_DESCRIPTORS[type];
  }
  return NULL;
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
  const uint8_t *base = reinterpret_cast<const uint8_t *>(e);
  return *reinterpret_cast<const bool *>(
      base + EFFECT_DESCRIPTORS[type].enabledOffset);
}
