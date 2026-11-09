# VocalSat - Parameter Specification

## Core Parameters

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| `drive` | Drive | Float | 0.0 - 24.0 | 6.0 | dB | Amount of pre-gain into the saturation stage (also drives the automatic output compensation) |
| `tone` | Tone | Float | -100.0 - 100.0 | 0.0 | % | Post-saturation tilt EQ: negative = darker/warmer, positive = brighter/more present |
| `mix` | Mix | Float | 0.0 - 100.0 | 100.0 | % | Dry/wet blend between the clean input and the saturated signal |

**Total exposed parameters: 3.** No hidden modes, no menus.

## Parameter Behavior

### Drive

- **Range:** 0.0 dB to +24.0 dB.
- **Scaling:** Linear in dB (perceptually even drive increase).
- **Default:** +6.0 dB (subtle, clearly audible thickening).
- **Smoothing:** ~20 ms to avoid zipper noise when automated.
- **Behavior:** Feeds the soft asymmetric saturation curve. As Drive increases, the internal auto-gain stage reduces output so perceived loudness stays approximately constant.
- **Units:** Decibels.

### Tone

- **Range:** -100% to +100%.
- **Scaling:** Linear, bipolar, center-detent at 0%.
- **Default:** 0% (neutral).
- **Smoothing:** ~20 ms.
- **Behavior:** Single tilt EQ placed after saturation.
  - Negative values attenuate the high band and gently reinforce the low band (warmer, darker, de-harshed).
  - Positive values attenuate the low band and lift the high band (brighter, more presence and air).
  - At 0% the tilt is bypassed (flat).
- **Units:** Percent (bipolar).

### Mix

- **Range:** 0% to 100%.
- **Scaling:** Linear.
- **Default:** 100% (fully saturated path).
- **Smoothing:** ~20 ms.
- **Behavior:** Equal-power or linear dry/wet crossfade between the clean input and the saturated output (topology decided during `/apc-plan`; linear is acceptable for correlated signals).
- **Units:** Percent.

## Internal (Non-Automatable) Behavior

These are implementation details, not user controls:

| Behavior | Purpose |
|---|---|
| Auto output compensation | Keeps output level roughly constant as `drive` changes |
| Internal oversampling (2x/4x) | Prevents aliasing from the nonlinear stage |
| DC blocker | Removes DC offset introduced by asymmetric saturation |
| Input/output metering | Drives the VU-style UI feedback only |

## Technical Requirements

### Real-Time Safety

- All continuous parameters must be smoothed for audio-thread use.
- No memory allocation, locks, or file I/O on the audio thread.
- Oversampling filters and all buffers pre-allocated on prepare/play.

### Parameter Persistence

- All three parameters saved with the plugin state.
- Fully automatable in VST3/AU/CLAP hosts.
- Parameter IDs must remain stable: `drive`, `tone`, `mix`.

### Host Compatibility

- Sample rates: 44.1, 48, 88.2, 96, 176.4, 192 kHz.
- Channel configurations: mono and stereo.
- Bypass must not click (crossfade or smoothed gain).

## Future Expansion

Not planned for v1. Possible later additions (would require a new `/apc-evolve` round):

- High-pass filter on the saturation input (de-mud).
- Sibilance-aware high-band drive trim.
- Preset browser.

Any future parameter must keep the three-knob philosophy intact.
