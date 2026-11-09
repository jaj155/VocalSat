# VocalSat - Creative Brief

## Hook

**"Analog warmth for vocals that cuts through the mix without ever sounding distorted."**

VocalSat is a vocal-first saturator: it adds the harmonic weight, presence and "glue" of a vintage tube channel strip, while automatic gain compensation and oversampling keep the result clean, level-matched and free of digital harshness.

## Vision

A focused, single-purpose saturation tool built around one question: *does the vocal sound better?*

- **Three controls only** - Drive, Tone, Mix. No menus, no modes, no decision fatigue.
- **Automatic output compensation** - as Drive rises, output gain is trimmed so the plugin stays level-matched and A/B comparison stays honest.
- **Vocal-tuned voicing** - the saturation curve and tone tilt are tuned around the 1-8 kHz presence region where vocals live, avoiding the fizzy top end of generic waveshapers.
- **Oversampled, anti-aliased** - internal oversampling keeps sibilance and air free of aliasing artifacts.
- **Vintage studio interface** - brushed metal, amber backlight, VU-style metering and large skeuomorphic knobs.

## Description

VocalSat processes the input through a soft, asymmetric saturation stage driven by the **Drive** control. The curve is deliberately gentle: low Drive values add body and low-mid weight, mid values add presence and forwardness, and high values push into a warm, compressed "driven console" character - never into hard clipping.

A single **Tone** tilt control rebalances the harmonic content after saturation: turning it down smooths and darkens the top end (de-harshing, warmth); turning it up restores air and intelligibility (presence, cut-through).

**Mix** blends the saturated signal with the clean input for parallel saturation, letting the same plugin work as a subtle bus glue or as an obvious effect.

The signal path is stereo-safe, click-free (all parameters smoothed), and does not allocate memory on the audio thread.

## Sonic Character

- **Low Drive (0-6 dB):** subtle harmonic thickening, body and low-mid warmth, almost transparent.
- **Mid Drive (6-14 dB):** audible presence and forwardness, classic tube-channel vocal sound, gentle dynamic compression.
- **High Drive (14-24 dB):** dense, creamy saturation with a compressed, "record-ready" attitude - still controlled, no hard edges.
- **Tone down:** darker, rounder, de-harshed; useful on bright or sibilant sources.
- **Tone up:** brighter, more present, more "expensive"; useful on dull or buried vocals.
- **Mix < 100%:** parallel saturation that preserves the natural dynamics of the dry take.

## Target Use Cases

1. **Lead vocals** - add weight and presence so the vocal sits in a dense mix.
2. **Backing vocals and stacks** - glue multiple takes together with shared harmonic color.
3. **Voiceover and podcast** - warm, broadcast-style presence without obvious distortion.
4. **Vocal bus** - gentle parallel saturation across the whole vocal group.
5. **Any source that needs warmth** - acoustic guitar, snare, bass DI (bonus, not the primary focus).

## Unique Selling Points

- **Vocal-first tuning** - the saturation curve is voiced for intelligibility, not for guitar-style distortion.
- **Auto gain compensation** - no volume-matching guesswork when automating Drive.
- **Oversampling** - clean sibilance and air, no aliasing "grain" at high Drive.
- **Three knobs, zero learning curve** - instant results, fully automatable.
- **Vintage studio identity** - a UI that looks like hardware and reads clearly at a glance.

## Design Philosophy

**Vintage studio hardware, modern clarity.** The interface should feel like a piece of 1970s outboard gear sitting on a console:

- Brushed-metal panel with subtle wear and warm amber backlighting.
- Large, chunky knobs with clear pointer lines and engraved-style labels.
- A VU-style meter for input/output activity (visual feedback, not a required control).
- High contrast, legible typography; no clutter and no hidden pages.
- Everything visible in a single window at typical plugin sizes (e.g. 720x480), scaling cleanly on hi-DPI displays.

## Technical Foundation

- Mono/stereo operation, 44.1-192 kHz host sample rates.
- Soft asymmetric waveshaping stage with internal oversampling (2x/4x, decided during `/apc-plan`).
- Automatic output gain compensation linked to the Drive amount.
- Tilt EQ (single control) applied post-saturation.
- Dry/wet parallel mix.
- Parameter smoothing on all continuous controls; no allocations or locks on the audio thread.
- Full state persistence and DAW automation for Drive, Tone and Mix.

## Out of Scope (v1)

- Multiband saturation, EQ curves, compressor section, presets browser, mid/side processing.
- Any feature that requires more than the three core controls.
