# Pedal display UI concepts

These five concepts use only information that the live firmware already exposes, except where noted. Every mockup is drawn at the display's native 480×320 aspect ratio and is intended to survive RGB565 conversion.

## 01 — Chromatic Deck (recommended)

The safest high-quality direction for a live pedal: a stable 2×2 spatial map matching switches A–D, with color used as a strong identity and a full fill for the active toggle state. It remains readable from standing distance and maps cleanly to dirty-region rendering.

Implementation: four fixed quadrant regions, a 42 px status header, and a 44 px expression footer. No new runtime state is required.

## 02 — Analog Console

A physical-instrument look based on rack labels, channel strips, indicator lamps, and a segmented expression meter. It feels like dedicated music hardware rather than a generic app.

Implementation: four fixed columns. Circles and meter segments are simple integer primitives. No image assets or new runtime state are required.

## 03 — Performance Focus

The most recently operated switch becomes the hero while the other three remain visible in a side rail. This has the strongest active-state feedback and the largest preset name.

Implementation: requires adding `activeSwitch` to `LiveView`, updated on switch activation. A switch change redraws the hero and side rail, so it moves more pixels than the fixed-grid concepts.

## 04 — Signal Flow

A vertical chain view that reads like a signal path. The selected preset is a full-width interruption in the chain; inactive choices are quieter but still clear.

Implementation: four horizontal rows and fixed state badges. It uses only current runtime data and makes each row independently redrawable.

## 05 — Gig Poster

The boldest direction: borderless color fields and oversized condensed type. It treats the display like stage graphics, with almost no interface chrome.

Implementation: four fixed quadrant fills plus a footer. This is computationally the simplest concept and has the best fallback behavior if typography is reduced to a 1-bit mask.

## Rendering plan

- Keep the current 480×320 RGB565 framebuffer: 307,200 bytes.
- Store font glyph atlases in flash, not RAM. Convert an open-source condensed display face and a highly legible small face into 4-bit alpha masks at build time. Blend those masks directly into RGB565; this gives visibly smoother type without LVGL.
- Add a small renderer layer for filled rectangles, lines, circles, clipped text, and alpha-mask glyphs. These five concepts need no general widget framework.
- Draw into the existing framebuffer and use the panel's proven full-screen address window. This specific module misplaces partial vertical windows, so correctness takes priority over regional transfer size.
- Hold chip-select active for the frame and stream it four scanlines at a time. MIDI, USB, switch scanning, and watchdog work run between these bounded transfer steps while the controller remains in continuous RAM-write mode.
- At the current 8 MHz SPI clock, a full frame takes about 307 ms of total wire time, but no individual display service step blocks the control loop for more than roughly four scanlines. A future optimization can retest higher SPI clocks on the real unit in measured steps.
- Keep state readable without color: active presets also receive a solid fill plus an explicit `ON` or `ACTIVE` label.
- Avoid continuous decorative animation at 8 MHz. State changes and the expression meter update incrementally without blocking the control loop for a complete frame.

## Font direction

Use a condensed, wide-aperture face for preset names and large switch letters, paired with a very legible regular-width face for status text. The production font set only needs uppercase letters, digits, basic punctuation, and the characters accepted by the configuration format, keeping flash cost controlled.
