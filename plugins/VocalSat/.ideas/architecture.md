# VocalSat - DSP Architecture Specification

## Overview

VocalSat is a vocal-first saturation plugin built as a short, linear signal chain:
pre-gain into a nonlinear stage, tone shaping after it, then a parallel mix and
level compensation. The design favors predictability and low CPU over algorithmic
complexity: there is exactly one nonlinearity, one tone control and one mixer.

**Target formats:** VST3 + Standalone (CLAP not opted in).
**Channel configs:** mono and stereo.
**Sample rates:** 44.1 - 192 kHz.
**UI framework:** WebView2 (HTML/CSS/JS).

---

## Core Components

1. **Parameter layer (APVTS)**
   - `drive` (0-24 dB), `tone` (-100..+100%), `mix` (0-100%).
   - One `juce::SmoothedValue` per continuous parameter (~20 ms ramp).
   - Bypass flag with smoothed crossfade (no click).

2. **Drive pre-gain**
   - `preGain = 10^(drive_dB / 20)` (range 1.0 to ~15.85).
   - Smoothed per sample.

3. **DC blocker**
   - First-order high-pass at ~20 Hz before the nonlinearity.
   - `y[n] = x[n] - x[n-1] + R * y[n-1]`, `R = exp(-2*pi*20/fs)`.
   - Per-channel state; reset on prepare.

4. **Oversampling stage**
   - `juce::dsp::Oversampling<float>`, factor **4x**, polyphase IIR (low latency/CPU).
   - Latency reported via `setLatencySamples(oversampler.getLatencyInSamples())`.
   - Pre-allocated in `prepareToPlay`; no allocation in `processBlock`.

5. **Saturation core (single nonlinearity)**
   - Asymmetric soft clip, vocal-tuned (gentle knee, no hard edges):
     ```
     xd = x * preGain
     y  = (tanh(k * (xd + b)) - tanh(k * b)) / k
     ```
     with `b` = asymmetry bias (~0.08) to generate controlled even harmonics
     (2nd order = body/warmth) and `k` = curve steepness (~1.0-2.0, mildly
     increasing with Drive). The `- tanh(k*b)` term removes the DC offset that
     the asymmetry would otherwise introduce.
   - Operates at 4x the host rate; per-channel state.

6. **Auto gain compensation**
   - Static compensation curve `comp(drive)` calibrated so a -18 dBFS, 1 kHz
     sine keeps a roughly constant output RMS across the full Drive range.
   - Applied after downsampling, smoothed (~20 ms) so Drive automation is click-free.
   - Goal: A/B comparison at equal perceived loudness.

7. **Tone tilt EQ (post-saturation)**
   - Single bipolar control mapped to a tilt around a ~700 Hz pivot:
     `lowGain = -t * 4.5 dB`, `highGain = +t * 4.5 dB`, where `t = tone/100`.
   - Implemented as a low shelf (-4.5..+4.5 dB) plus high shelf (+4.5..-4.5 dB)
     at the pivot (or an equivalent one-pole tilt split).
   - At `tone = 0` the response is flat.

8. **Dry/wet mixer**
   - `out = dry * (1 - mix) + wet * mix` (linear crossfade; signals are correlated).
   - **Dry path must be delayed by the oversampler latency** so parallel mixing
     stays phase-coherent. Dry delay buffer sized in `prepareToPlay`.

9. **Output stage and metering**
   - Output gain = auto-compensation (no extra user control in v1).
   - Peak/RMS values published through `std::atomic<float>` for the WebView VU meter.
   - No hard limiter: the soft curve keeps peaks bounded by design.

---

## Processing Chain

```
Input
  -> Drive pre-gain (smoothed)
  -> DC blocker (20 Hz HPF)
  -> Oversample 4x  [upsample]
  -> Asymmetric saturation (tanh, bias)
  -> Oversample 4x  [downsample]
  -> Tone tilt EQ
  -> Dry/Wet mix  <-- dry path delayed by oversampler latency
  -> Auto gain compensation (smoothed)
  -> Output (+ metering)
```

---

## Parameter Mapping

| Parameter | Component | Function | Range |
|-----------|-----------|----------|-------|
| `drive` | Drive pre-gain + saturation core | Sets input level into the nonlinearity and harmonic density; also selects the auto-comp curve | 0.0 - 24.0 dB |
| `tone` | Tone tilt EQ | Bipolar tilt: negative = darker/warmer, positive = brighter/present | -100.0 - 100.0 % |
| `mix` | Dry/wet mixer | Blend between clean (delay-compensated) input and saturated signal | 0.0 - 100.0 % |

### Parameter smoothing

| Parameter | Smoothing |
|---|---|
| `drive` | ~20 ms (also drives the compensation ramp) |
| `tone` | ~20 ms |
| `mix` | ~20 ms |

---

## Latency and Phase

- Oversampling introduces latency; it is reported to the host and compensated in
  the dry path so `mix < 100%` does not comb-filter.
- No lookahead or additional delay beyond the oversampler.
- Bypass crossfades over ~10 ms to avoid clicks.

---

## Real-Time Safety

- All buffers, filters and oversamplers allocated in `prepareToPlay`.
- No locks, allocations, file I/O or logging on the audio thread.
- Parameters read from APVTS atomics; smoothing done per block/per sample.
- Metering uses atomics only.

---

## State Persistence

- APVTS state (`getStateInformation` / `setStateInformation`) covers all three
  parameters plus the bypass flag.
- Parameter IDs are stable: `drive`, `tone`, `mix`.
- No hidden state that affects audio; the plugin is fully recallable from APVTS.

---

## Complexity Assessment

**Score: 2 / 5 (Moderate)**

**Rationale:**
- Three parameters, one nonlinearity, one EQ, one mixer - conceptually simple.
- Moderate factors: 4x oversampling (latency + anti-alias care), latency
  compensation of the dry path, and an auto-gain curve that must be calibrated.
- No multi-band processing, no dynamics detection, no modulation, no analysis
  engine, no state machine - therefore well below Level 3.

**Implementation strategy:** single-pass (Score <= 2), with a short optimization
pass for real-time safety and a polish pass for edge cases.

---

## Out of Scope (v1)

Multiband saturation, EQ curves, compressor, preset browser, mid/side processing,
external sidechain. Any of these would require a new `/apc-evolve` round.
