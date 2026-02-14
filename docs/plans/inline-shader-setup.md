# Inline Shader Setup Functions

Move setup functions from `shader_setup_{category}` files into each effect's `.cpp`, eliminating 20 files.

## Motivation

With self-registering effects, the category shader_setup files are pure boilerplate. Each setup function is a one-liner delegating to the effect module. Moving them into the effect `.cpp` makes each effect fully self-contained.

## Scope

**Eliminate:** 10 `shader_setup_{category}.cpp` + 10 `shader_setup_{category}.h` (artistic, cellular, color, generators, graphic, motion, optical, retro, symmetry, warp)

**Keep:** `shader_setup.cpp` (table lookup, non-transform setups, multi-pass helpers), `shader_setup.h`

## Approach

- Move each `Setup*` function into its effect `.cpp`, above the `REGISTER_EFFECT` macro
- The macro already forward-declares the setup function, so no header needed
- Multi-pass helpers (`ApplyBloomPasses`, `ApplyAnamorphicStreakPasses`, `ApplyHalfResEffect`, `ApplyHalfResOilPaint`) stay in `shader_setup.cpp` or move into their respective effect `.cpp` files
- Remove 20 files from `CMakeLists.txt`
- Update add-effect skill to remove Phase 5 entirely
