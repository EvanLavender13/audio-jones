# Modulation Buses

Virtual named buses that combine multiple modulation sources with operators, producing a single output usable as a modulation source. Buses let users build composite modulation signals (e.g., "bass gated by LFO1", "treble multiplied by LFO3") and reuse them across presets.

## Classification

- **Category**: General (modulation engine architecture)
- **Pipeline Position**: Evaluated per frame between source update and route application

## References

- [SynthLab SDK: Modulation Matrix](https://www.willpirkle.com/synthlab/docs/html/mod_matrix.html) - C++ mod matrix with intensity stacking, multiply-accumulate routing
- [music-dsp: Modulation Matrix Implementation](https://music-dsp.music.columbia.narkive.com/XcVZk8Ij/modulation-matrix-implementation-trickery) - Control-rate update, normalized domain math, linear sum before domain conversion
- [Unreal Engine: Audio Modulation Control Buses](https://forums.unrealengine.com/t/has-anyone-figured-out-control-buses-audio-modulation-plugin/143566) - Named control buses with per-parameter mix operators, transfer functions
- [A-to-Synth: Modulation Matrix Thoughts](http://atosynth.blogspot.com/2025/01/modulation-matrix-thoughts.html) - Sparse route representation, chaining, add-vs-multiply design question
- [Matrix Modular Synthesis - Nathan Ho](https://nathan.ho.name/posts/matrix-modular-synthesis/) - N x N matrix-vector multiply pattern for arbitrary routing with feedback

## Algorithm

### Bus Structure

Each bus has:
- A unique string name (user-assigned, used as serialization key)
- Two input slots, each selecting a `ModSource` (audio band, LFO, or another bus's output)
- An operator that combines the two inputs
- A single float output in [-1, +1] range

### Operators

**Combinators (2 inputs A, B):**

| Operator | Formula | Use case |
|----------|---------|----------|
| Add | `clamp(A + B, -1, 1)` | Layer two sources |
| Multiply | `A * B` | Amplitude modulation |
| Min | `min(A, B)` | Take the quieter of two signals |
| Max | `max(A, B)` | Take the louder of two signals |
| Gate | `A > 0 ? B : 0` | Source B only passes when A is positive |
| Crossfade | `lerp(A, B, 0.5)` | Blend equally (crossfade amount could be a param) |
| Difference | `abs(A - B)` | Detects divergence between two sources |

### Processing Order

1. `ModSourcesUpdate()` fills `values[]` with raw audio and LFO outputs (existing)
2. Evaluate buses in registration order. Each bus reads its inputs from `values[]` (which may include prior bus outputs if chained), applies its operator, and writes its output to a bus output slot
3. `ModEngineUpdate()` applies routes to parameters (existing)

For chained buses (bus B reads bus A's output), evaluation order matters. Buses evaluated later can read outputs from buses evaluated earlier in the same frame. If a cycle exists (A reads B, B reads A), the later bus reads the previous frame's value from the earlier one. This produces a one-frame delay on cycles, not a crash or infinite loop.

### Integration with ModSource

Bus outputs become available as modulation sources. Extend the `ModSource` enum:

```
MOD_SOURCE_BUS1 = 18,
MOD_SOURCE_BUS2 = 19,
...
MOD_SOURCE_BUS8 = 25,  // 8 bus slots
MOD_SOURCE_COUNT = 26
```

The `ModSources::values[]` array grows to include bus output slots. The modulation route popup adds a "Buses" category alongside Bands, Spectral, Features, and LFOs.

### Bus Count

8 buses (matching the 8 LFOs). This is enough for complex setups without excessive UI. Stored as a fixed-size array like LFOs.

### Serialization

Buses serialize as a named array in the preset JSON, alongside the existing `lfo` and `modRoutes` sections:

```json
{
  "modBuses": [
    {
      "name": "gated bass",
      "enabled": true,
      "inputA": 0,
      "inputB": 4,
      "op": 4
    }
  ]
}
```

Since buses are named and self-contained, users can share bus recipes across presets by copying entries.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| enabled | bool | - | false | Bus on/off |
| name | char[32] | - | "" | User-assigned label |
| inputA | int (ModSource) | 0-25 | MOD_SOURCE_BASS | First input source |
| inputB | int (ModSource) | 0-25 | MOD_SOURCE_LFO1 | Second input source |
| op | int (BusOp) | 0-6 | BUS_OP_MULTIPLY | Combining operator |

No per-bus `amount` or `curve` -- those live on the route that reads the bus output, same as any other source.

## Modulation Candidates

Envelope and slew processor params (attack, release, hold, threshold, lagTime, riseTime, fallTime) are modulatable. Register as `"bus<N>.attack"`, `"bus<N>.lagTime"`, etc. Combinator buses have no modulatable params.

## UI Design

### Window

New docking tab: **Buses** (alongside LFOs, Analysis, Drawables, Effects).

8 bus slots, always visible, using the Module Strip pattern (`DrawModuleStripBegin/End`). Disabled buses are dimmed but fully rendered -- same density model as LFOs. No `DrawGroupHeader` -- window title suffices.

Accent colors cycle identically to LFOs: index % 3 maps to Cyan/Magenta/Orange.

### Module Strip Layout

Each bus is a Module Strip. Content adapts based on operator type:

**Combinator bus (2 inputs, no extra params):**

```
|  * BUS 1  gated bass                        [ Gate         v]    |
|                                                                   |
|  A [ Bass         v]    B [ LFO 1        v]                     |
|                                                                   |
|  +---------------------------------------------+  ||             |
|  | ~../`\.._  _./'\./`\..  _./'\../`\..  __./` |  ||             |
|  +---------------------------------------------+  ||             |
```

**Row 1**: Enable dot, "BUS N", editable name (inline InputText), operator combo (right-aligned)
**Row 2**: Input A combo (labeled "A"), Input B combo (labeled "B")
**Row 3**: Output history graph (~2s scrolling waveform) + vertical meter bar

**Envelope Follow bus (1 input + params):**

```
|  * BUS 2  smooth beat                       [ Env Follow    v]   |
|                                                                   |
|  Input [ Beat        v]                                          |
|                                                                   |
|  --- Envelope -------------------------------------------------- |
|  Attack   [======|==========]  0.01 s                            |
|  Release  [==========|======]  0.30 s                            |
|                                                                   |
|  +---------------------------------------------+  ||             |
|  | _/--\_____/---\____/-\_______/----\_________ |  ||             |
|  +---------------------------------------------+  ||             |
```

**Row 1**: Same header. **Row 2**: Single "Input" combo (no B -- hidden for processors).
**Row 3**: `SeparatorText("Envelope")` + Attack/Release sliders.
**Row 4**: History graph + meter.

**Envelope Trigger bus** adds Hold and Threshold sliders below Release.

**Slew bus (symmetric):**

```
|  * BUS 4  smooth treble                     [ Slew (Exp)    v]   |
|                                                                   |
|  Input [ Treble      v]                                          |
|                                                                   |
|  --- Slew ---------------------------------------------------    |
|  Lag Time  [========|=======]  0.20 s                            |
|                                                                   |
|  +---------------------------------------------+  ||             |
|  | ~~~-----~~~~~------~~~~------~~~~~---------- |  ||             |
|  +---------------------------------------------+  ||             |
```

**Slew bus (asymmetric):** Checkbox `[x] Asymmetric` swaps Lag Time for Rise Time + Fall Time pair.

**Disabled bus:** All text pushed to `ImGuiCol_TextDisabled`. Outline-only enable dot. Empty history graph. Name hidden.

### Adaptive Content Rules

| Operator | Input B | Params Section | Params |
|----------|---------|----------------|--------|
| Add, Multiply, Min, Max, Gate, Crossfade, Difference | Shown | None | -- |
| Env Follow | Hidden | "Envelope" | Attack, Release |
| Env Trigger | Hidden | "Envelope" | Attack, Release, Hold, Threshold |
| Slew (Exp), Slew (Linear) | Hidden | "Slew" | Lag Time (or Rise/Fall if asymmetric) |

### Widget Details

**Operator combo**: `ImGui::Combo`, right-aligned. 11 entries with `ImGui::Separator()` between categories (Combinators | Envelope | Slew).

**Input combos**: `ImGui::Combo` with all ModSources, grouped with separators (Bands | Spectral/Features | LFOs | Buses). Bus entries at the bottom enable chaining.

**Processor params**: `ModulatableSlider` for envelope/slew params (attack, release, hold, threshold, lagTime, riseTime, fallTime). Combinator buses have no params.

| Param | Format | Range |
|-------|--------|-------|
| Attack | `"%.3f s"` | 0.001 - 2.0 |
| Release | `"%.2f s"` | 0.01 - 5.0 |
| Hold | `"%.2f s"` | 0.0 - 2.0 |
| Threshold | `"%.2f"` | 0.0 - 1.0 |
| Lag Time | `"%.2f s"` | 0.01 - 5.0 |
| Rise/Fall Time | `"%.2f s"` | 0.01 - 5.0 |

**History graph**: Ring buffer of ~120 values (2s at 60fps), rendered with `AddPolyline()` into ~32px height region. Background via `DrawGradientBox()`. Color: accent color.

**Output meter**: 4px wide vertical bar, right edge. Center-fill for bipolar (combinators), bottom-fill for unipolar (envelope/slew). Color: accent color.

**Name field**: `ImGui::InputText`, minimal styling, no border when unfocused. Shows "Bus N" placeholder when empty. Only visible when enabled. Max 31 chars.

### Modulation Route Popup Integration

The existing diamond-indicator popup gains a "Buses" category:

```
--- Buses ---
[1] [2] [3] [4]
[5] [6] [7] [8]
```

Bus buttons show the bus name as tooltip on hover. Button color matches the bus's accent color.

### Full Window Mockup

```
+------------------------------------------------------------------+
|  Buses                                                      _ x  |
|                                                                   |
| | * BUS 1  gated bass                  [ Gate         v]         |
| |  A [ Bass       v]   B [ LFO 1      v]                        |
| |  +----------------------------------------+  ||                |
| |  | _../'\._  __./'\./'\.__  _../'\._  __. |  ||                |
| |  +----------------------------------------+  ||                |
|                                                                   |
| | * BUS 2  smooth beat                 [ Env Follow    v]        |
| |  Input [ Beat      v]                                          |
| |  --- Envelope ----------------------------------------         |
| |  Attack   [==|========]  0.010 s                                |
| |  Release  [========|==]  0.300 s                                |
| |  +----------------------------------------+  ||                |
| |  | _/‾\_____/‾‾\____/‾\______/‾‾‾\_____ |  ||                |
| |  +----------------------------------------+  ||                |
|                                                                   |
| | * BUS 3  beat pulse                  [ Env Trigger   v]        |
| |  Input [ Beat      v]                                          |
| |  --- Envelope ----------------------------------------         |
| |  Attack    [==|=======]  0.010 s                                |
| |  Release   [========|=]  0.300 s                                |
| |  Hold      [|========]   0.000 s                                |
| |  Threshold [===|======]  0.300                                  |
| |  +----------------------------------------+  ||                |
| |  | _/\_________/\__/\______________/\____ |  ||                |
| |  +----------------------------------------+  ||                |
|                                                                   |
| | * BUS 4  smooth treb                 [ Slew (Exp)    v]        |
| |  Input [ Treble    v]                                          |
| |  --- Slew -------------------------------------------          |
| |  Lag Time [======|====]  0.200 s                                |
| |  +----------------------------------------+  ||                |
| |  | ~~-----~~~~~------~~~~------~~~~~----- |  ||                |
| |  +----------------------------------------+  ||                |
|                                                                   |
| | o BUS 5                              [ Multiply      v]        |
| |  A [ Bass       v]   B [ LFO 1      v]                        |
| |  +----------------------------------------+  ||                |
| |  |                                        |  ||                |
| |  +----------------------------------------+  ||                |
|                                                                   |
| | o BUS 6                              [ Multiply      v]        |
| |  A [ Bass       v]   B [ LFO 1      v]                        |
| |  +----------------------------------------+  ||                |
| |  |                                        |  ||                |
| |  +----------------------------------------+  ||                |
|                                                                   |
| | o BUS 7                              [ Multiply      v]        |
| |  A [ Bass       v]   B [ LFO 1      v]                        |
| |  +----------------------------------------+  ||                |
| |  |                                        |  ||                |
| |  +----------------------------------------+  ||                |
|                                                                   |
| | o BUS 8                              [ Multiply      v]        |
| |  A [ Bass       v]   B [ LFO 1      v]                        |
| |  +----------------------------------------+  ||                |
| |  |                                        |  ||                |
| |  +----------------------------------------+  ||                |
|                                                                   |
+------------------------------------------------------------------+
```

## Notes

- Bus evaluation adds one loop over 8 buses per frame -- negligible cost
- The `ModSources::values[]` array grows from 18 to 26 entries. All existing code indexes by enum, so no breakage
- Crossfade could gain a `mix` parameter (0-1 blend position) in a future pass. For now, fixed at 0.5
- Gate threshold is 0 (positive = open). A threshold parameter could be added later
- Bus names are for display and serialization only -- routes reference buses by their `MOD_SOURCE_BUS<N>` enum index
