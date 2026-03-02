# Multi-Pass Effect Dispatch Generalization

## Problem

After the ping-pong generalization removes the attractor_lines and fireworks `else if` branches, three effect-specific branches remain in the transform dispatch loop (`render_pipeline.cpp:305-347`):

```cpp
} else if (effectType == TRANSFORM_BLOOM) {
    ApplyBloomPasses(pe, src, &writeIdx);
    RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader, entry.setup);
} else if (effectType == TRANSFORM_ANAMORPHIC_STREAK) {
    ApplyAnamorphicStreakPasses(pe, src);
    RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader, entry.setup);
} else if (effectType == TRANSFORM_OIL_PAINT) {
    ApplyHalfResOilPaint(pe, src, &writeIdx);
}
```

Each new multi-pass effect would add another branch. The ping-pong `render` pointer solves generators but not these.

## Analysis of Each Branch

### Bloom

`ApplyBloomPasses(pe, src, &writeIdx)` performs downsample → blur → upsample through bloom mip chain. The bloom composite shader then blends the result via the standard `RenderPass`. Pattern: **pre-pass + compositor**.

Signature: `void(PostEffect*, RenderTexture2D*, int*)`

### Anamorphic Streak

`ApplyAnamorphicStreakPasses(pe, src)` performs horizontal threshold → progressive blur chain into streak textures. The composite shader blends via standard `RenderPass`. Pattern: **pre-pass + compositor**.

Signature: `void(PostEffect*, RenderTexture2D*)`

### Oil Paint

`ApplyHalfResOilPaint(pe, src, &writeIdx)` downscales, applies the oil paint shader at half res, then upscales back. It does NOT follow with a separate `RenderPass` — the entire pipeline is self-contained. Pattern: **self-contained pass** (similar to `ApplyHalfResEffect` but with a custom downscale/upscale flow).

Signature: `void(PostEffect*, RenderTexture2D*, const int*)`

## Key Differences from Ping-Pong Generators

1. **Varying signatures**: Bloom takes mutable `writeIdx`, anamorphic ignores it, oil paint takes const `writeIdx`. The ping-pong solution worked because all generators share `void(PostEffect*)`.

2. **Post-pass behavior differs**: Bloom and anamorphic need the standard `RenderPass` afterward. Oil paint does not.

3. **Source texture dependency**: All three need `source` (the current chain read texture), which ping-pong generator wrappers don't.

## Proposed Solution

Reuse the same `render` field from the ping-pong generalization, but broaden its contract:

```
void (*render)(PostEffect *pe) = nullptr;
```

The key insight: `src` and `writeIdx` are already available on `PostEffect` if we store them as transient frame state, OR each wrapper can access them through existing fields. But currently `src` and `writeIdx` are local variables in the dispatch loop — they aren't on `PostEffect`.

### Option A: Store Loop State on PostEffect

Add two transient fields to `PostEffect`:
```
RenderTexture2D *currentSrc;
int currentWriteIdx;
```

Set them at the top of each loop iteration. The `render` wrapper reads them. After render returns, read them back (in case the wrapper modified writeIdx, as bloom does).

Pros: Uniform `void(PostEffect*)` for all wrappers. Cons: Transient mutable state on a long-lived struct.

### Option B: Wider Render Signature

Change the render pointer to:
```
void (*render)(PostEffect *pe, RenderTexture2D *src, int *writeIdx);
```

Each wrapper receives the loop locals directly.

Pros: No transient state pollution. Cons: Different signature from the ping-pong `render` field (which is `void(PostEffect*)`).

### Option C: Leave As-Is

The three remaining branches are stable, distinct effects unlikely to spawn copies. The ping-pong generalization handles the growing category (generators). These three are one-offs.

Pros: No change needed. Cons: The branches remain.

## Recommendation

**Option C** for now. The three remaining branches represent three genuinely different dispatch patterns (pre-pass+composite, pre-pass+composite, self-contained). Generalizing them into one pointer would either require transient state on PostEffect or a wider signature that doesn't match the ping-pong render field. The benefit is small — each pattern has at most one effect using it, and adding a second bloom-style or oil-paint-style effect is unlikely.

If a second multi-pass pre-pass effect is added, revisit Option B with a dedicated `prePass` pointer separate from the ping-pong `render` pointer.

## Affected Files (if pursued)

- `src/render/post_effect.h` — add transient frame state (Option A) or no change (Option B)
- `src/config/effect_descriptor.h` — widen render signature (Option B) or no change (Option A reuses existing)
- `src/render/render_pipeline.cpp` — replace branches with dispatch
- `src/render/shader_setup.cpp` — move `ApplyBloomPasses`, `ApplyAnamorphicStreakPasses`, `ApplyHalfResOilPaint` into effect wrappers

## Scope

Deferred. Revisit when a new multi-pass effect is added.
