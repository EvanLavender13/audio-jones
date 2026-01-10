# Enum Modernization

Refactor all `typedef enum` declarations to C++11 `enum class` for type safety and scoped naming.

## Scope

Convert 10 enums from:
```c
typedef enum {
    CHANNEL_LEFT,
    CHANNEL_RIGHT,
    ...
} ChannelMode;
```

To:
```cpp
enum class ChannelMode {
    Left = 0,
    Right = 1,
    ...
};
```

## Files

- `src/audio/audio_config.h` - ChannelMode
- `src/automation/mod_sources.h` - ModSourceType
- `src/automation/modulation_engine.h` - ModCurveType
- `src/config/drawable_config.h` - DrawableType, DrawablePath
- `src/config/effect_config.h` - TransformEffectType
- `src/config/lfo_config.h` - LFOShape
- `src/render/blend_mode.h` - BlendModeType
- `src/render/color_config.h` - ColorMode
- `src/render/profiler.h` - ProfileZoneId
- `src/simulation/attractor_flow.h` - AttractorType

## Considerations

- Add explicit `= 0, = 1, ...` values for shader uniform compatibility
- Update all usage sites to scoped syntax (`ChannelMode::Left`)
- Add explicit `(int)` casts where enums pass to shaders
- JSON serialization continues to work (nlohmann handles enum class)
