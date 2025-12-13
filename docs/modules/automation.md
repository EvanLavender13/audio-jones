# Automation Module

> Part of [AudioJones](../architecture.md)

## Purpose

Generates periodic waveforms for parameter animation. The LFO oscillator modulates post-effect rotation at configurable rate and shape.

## Files

- `src/automation/lfo.h` - LFO state and processing API
- `src/automation/lfo.cpp` - Waveform generation (sine, triangle, saw, square, sample-hold)

## Function Reference

| Function | Purpose |
|----------|---------|
| `LFOStateInit` | Clears phase to 0, sets random initial held value |
| `LFOProcess` | Advances phase by rate * deltaTime, generates output (-1 to 1) |

## Types

### LFOState

| Field | Type | Description |
|-------|------|-------------|
| `phase` | `float` | Current position in cycle (0.0-1.0) |
| `currentOutput` | `float` | Last computed output (-1.0 to 1.0) |
| `heldValue` | `float` | Random value for sample-hold waveform |

## Data Flow

1. **Entry:** `LFOConfig` rate and waveform type from config module
2. **Transform:** Phase accumulation, waveform lookup
3. **Exit:** Normalized output (-1 to 1) to post-effect rotation calculation
