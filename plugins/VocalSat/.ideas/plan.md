# VocalSat - Implementation Plan

## Complexity Score: 2 / 5 (Moderate)

## Framework & Format Decisions

| Decision | Value | Rationale |
|---|---|---|
| UI framework | **webview** | The vintage-studio concept (VU meter, skeuomorphic knobs, amber glow) is far easier and richer in HTML/CSS/JS; also the project default from setup. |
| Formats | **VST3 + Standalone** | CLAP not opted in for v1; can be added later without code changes. |
| Implementation strategy | **single-pass** | Score <= 2: the whole chain can be built in one implementation session. |

---

## Implementation Strategy

### Single-Pass Implementation

Build the full chain in one session, in this order:

1. **Project scaffolding**
   - [ ] Plugin target + CMake wiring (via APC templates, WebView2 enabled)
   - [ ] APVTS with `drive`, `tone`, `mix` (ranges/defaults per `parameter-spec.md`)
   - [ ] Bypass parameter

2. **Core signal path**
   - [ ] Drive pre-gain with `SmoothedValue`
   - [ ] DC blocker (20 Hz, per-channel state)
   - [ ] 4x oversampler (`juce::dsp::Oversampling`), prepared with max block size
   - [ ] Asymmetric tanh saturation with bias + DC correction
   - [ ] Latency reporting and dry-path delay buffer

3. **Tone and mix**
   - [ ] Tilt EQ (low/high shelves around ~700 Hz, +/-4.5 dB)
   - [ ] Dry/wet mixer using the delay-compensated dry path

4. **Gain compensation and metering**
   - [ ] `comp(drive)` curve (calibrated on a -18 dBFS 1 kHz sine)
   - [ ] Smoothed compensation gain
   - [ ] Peak/RMS atomics for the VU meter

5. **UI integration**
   - [ ] WebView2 editor loading `Design/index.html`
   - [ ] Parameter binding (drive/tone/mix) and meter updates
   - [ ] Resizable window, default ~720x480, hi-DPI aware

6. **Optimization**
   - [ ] No allocation/locks on the audio thread
   - [ ] `prepareToPlay` allocates everything; `releaseResources` cleans up
   - [ ] CPU check at 4x oversampling, 48 kHz, stereo

7. **Polish**
   - [ ] Click-free bypass and parameter automation
   - [ ] Mono and stereo configurations
   - [ ] State save/restore (APVTS)
   - [ ] Edge cases: silence, very hot input, 192 kHz

---

## Dependencies

**Required JUCE modules:**
- `juce_audio_basics`
- `juce_audio_processors`
- `juce_dsp` (oversampling, filters)
- `juce_gui_basics`
- `juce_gui_extra` (WebView2)

**Optional / build-time:**
- `juce_audio_utils` (standalone wrapper helpers)
- `@juce-framework/webview` or `native/typescript/webview-interop` for the JS bridge
- WebView2 SDK (NuGet) - already installed during `/apc-setup`

**No external DSP libraries.** Everything is JUCE + standard C++.

---

## Risk Assessment

### High Risk
- **Aliasing at high Drive.** Mitigated by 4x oversampling and a gentle curve;
  verify with a high-frequency sine sweep at 20 dB Drive.
- **Latency/phase in parallel mix.** The dry path must be delayed by exactly the
  oversampler latency; a mismatch causes comb filtering at partial Mix.
- **Auto-gain calibration.** A badly tuned curve makes Drive unusable for A/B;
  calibrate numerically before shipping the default.

### Medium Risk
- **Parameter smoothing / zipper noise** on fast automation.
- **DC offset** from asymmetric saturation (mitigated by the bias subtraction +
  pre HPF; verify with an analyzer).
- **WebView2 bridge reliability** (parameter sync, meter updates at 30-60 Hz).

### Low Risk
- Tilt EQ implementation (standard shelf filters).
- Dry/wet mixer.
- APVTS persistence and automation.
- Mono/stereo channel handling.

---

## Acceptance Criteria (for `/apc-test`)

- [ ] Plugin builds as VST3 + Standalone with no warnings that affect audio.
- [ ] All three parameters automate smoothly (no zipper, no clicks).
- [ ] `mix = 100%` and `mix = 0%` are level-consistent (auto-gain works).
- [ ] `mix = 50%` shows no comb filtering on a 1 kHz sine.
- [ ] High-frequency sweep at max Drive shows no obvious aliasing.
- [ ] Bypass is click-free.
- [ ] State saves/restores in the host.
- [ ] CPU is acceptable at 4x oversampling, 48 kHz, stereo.

---

## Next Phase

`/apc-design VocalSat` - create the WebView mockups, style guide and `Design/index.html`.
