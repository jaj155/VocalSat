# Style Guide v1 — VocalSat

## Concept

**Vintage studio outboard, modern clarity.** A warm brushed-metal panel with ivory
silk-screen labels, amber backlighting and a large analog VU meter. The interface
should feel like a 1970s channel-strip module, not a software dashboard.

## Color Palette

| Token | Hex | Usage |
|---|---|---|
| `--plate` | `#2a2621` | Base plate |
| `--plate-2` | `#34302a` | Section panels, knob bodies |
| `--surface` | `#121010` | Letterbox / surround |
| `--inset` | `#0d0b09` | Recessed wells, LED screens |
| `--rule` | `#5c5245` | 1px control borders |
| `--rule-2` | `#403930` | Panel borders |
| `--ink` | `#f4e9d4` | Primary text |
| `--ink-2` | `#b9a888` | Labels, secondary text |
| `--accent` | `#ffb347` | Knob pointer, LED on, readout text |
| `--grip-ink` | `#6d5f4b` | Resize gripper |
| `--day-meter-face` | `#efe4c8` | VU meter face |
| `--meter-ink` | `#3b3327` | VU scale, needle and text |
| `--meter-accent` | `#b8862f` | VU safe zone |
| `--meter-brand` | `VU` | VU center label |

### Contrast

- Primary text `--ink` on `--plate` ≈ 11:1 — passes WCAG AAA for large text.
- Secondary `--ink-2` on `--plate` ≈ 5.4:1 — passes AA for normal text.
- Amber `--accent` is reserved for active/readout states; never use it for body copy.

## Typography

| Role | Font | Size | Weight | Case / spacing |
|---|---|---|---|---|
| UI / labels | `"Bahnschrift", "Segoe UI", ui-sans-serif, sans-serif` | 0.625 rem | 600 | UPPERCASE, 0.12em |
| Values / readouts | same | 0.8125 rem | 600 | tabular numerals |
| Micro (footer) | same | 0.4375 rem | 600 | UPPERCASE, 0.1em |
| Brand | same | 0.625 rem (catalog label) | 600 | UPPERCASE |

Legibility floors come from the catalog: `max(9px, …)` micro, `max(10px, …)` label,
`max(11px, …)` readout. Never shrink text below these.

## Spacing & Geometry

- Grid: 1 rem = 1 square cell; 45 x 30 cells on the plate.
- All cells snap to 0.25 rem.
- Standard gutter between knob columns: 0.5 rem.
- Panel padding: 1.5 rem minimum from section edge to content.
- Section radius: `--r-panel` (6px); control radius: `--r-control` (4px).
- No `px` sizing in authored layout — only hairlines (1px) and catalog internals.
- **Knob cells:** keep height ≤ width + ~0.85 rem. The catalog dial is `flex:1; aspect-ratio:1`,
  so a tall cell makes the dial grow wider than its column and overlap the neighbor
  (this was the v1 defect: 6 x 8.5 cells produced ~7.6 rem dials).

## Control Visual Styles

### Rotary knob (`knob`)

- Circular dial with radial gradient from `--plate-2` to `--inset`.
- 1px `--rule` border and `--sh-raised` shadow.
- Amber pointer (`--accent`), 0.09 rem wide, anchored at bottom center, rotated -135° at minimum.
- Label below the dial: uppercase `--ink-2`.
- Hover: pointer brightens to `#ffd28a`; active: dial shadow compresses slightly.
- Drag: vertical drag adjusts value; double-click resets to default.

### LED screen (`screen`)

- Recessed `--inset` background, `--sh-recess` inner shadow.
- Amber `--accent` text, tabular numerals, 0.08em letter-spacing.
- Shows the live parameter value: `6.0 dB`, `0 %`, `100 %`.

### Panel switch (`psw`)

- Rectangular `--plate-2` body, 1px `--rule` border.
- 0.5 rem LED: dim `--ink-2` at 35% opacity when off; `--accent` with a soft glow when on.
- Label `BYPASS`, uppercase 0.5 rem.

### VU meter (`canvas` kind `vu-meter`)

- Cream face (`--day-meter-face`) with ivory-to-cream vertical gradient.
- Dark scale marks and needle (`--meter-ink`), safe zone in `--meter-accent`, red overload zone.
- **Numbered scale:** only `-20, -10, -7, -3, 0, +3`. Minor ticks (-15, -5, -2, -1, +1, +2)
  stay unlabeled so the crowded top of the arc never stacks numbers.
- Center label: `VU` over `CHANNEL VOCAL` (`--meter-brand`, `--meter-ink`).
- Needle rests between `-3` and `0` in the preview and is shorter than the label ring,
  so it never crosses a number or the center text; production needle follows live RMS.
- Pivot sits at 0.90 of canvas height so the pivot cap is fully visible (no bottom clipping).
- Canvas must stay wide and short (~2:1); a tall canvas leaves the dial stranded at the bottom.

### Resize gripper

- In-page bottom-right triangle with `--grip-ink` hatching, 2.5 rem hit area.
- Cursor `nwse-resize`; drag forwards deltas to the host.

## UI States

| State | Description |
|---|---|
| **Default** | Drive 6.0 dB, Tone 0%, Mix 100%, bypass off. |
| **Subtle warmth** | Drive 3 dB, Tone -15%, Mix 60% — meter around -7 dB. |
| **Presence push** | Drive 10 dB, Tone +25%, Mix 100% — meter around -3 dB. |
| **Driven** | Drive 20 dB, Tone +10%, Mix 100% — meter peaking near 0 dB. |
| **Bypassed** | Bypass LED off, controls dimmed to 60% opacity, meter shows dry signal. |
| **Hover** | Pointer/LED brighten; no layout shift. |
| **Focus (keyboard)** | 2px `--accent` outline offset 2px around the control. |

## Motion

- Parameter changes animate the knob pointer over ~120 ms.
- Meter needle uses a fast attack / slow release ballistics (~10 ms / 300 ms).
- No decorative animations; motion only communicates signal or interaction.

## Assets

- No external images or icon fonts in v1 — all visuals come from the APC catalog
  and the token palette.
- Branding is typographic (`VOCALSAT` + `ANALOG WARMTH`); no logo file required.

## Do / Don't

- **Do** keep all three knobs on the same baseline and column width.
- **Do** use amber only for active/readout elements.
- **Don't** add drop shadows with colored tints, gradients behind text, or glow on body copy.
- **Don't** introduce controls or panels that are not in `v1-ui-map.json`.
