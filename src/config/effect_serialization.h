#ifndef EFFECT_SERIALIZATION_H
#define EFFECT_SERIALIZATION_H

#include "effect_config.h"
#include "render/color_config.h"
#include <nlohmann/json_fwd.hpp>

void to_json(nlohmann::json &j, const ColorConfig &c);
void from_json(const nlohmann::json &j, ColorConfig &c);
void to_json(nlohmann::json &j, const EffectConfig &e);
void from_json(const nlohmann::json &j, EffectConfig &e);

#endif
