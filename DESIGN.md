---
name: MIDI Pedal — Chromatic Deck
description: A saturated, glanceable stage interface built around the physical A–D switch map.
colors:
  stage-black: "#071013"
  status-surface: "#0D1A1F"
  deck-surface: "#102229"
  deck-white: "#F2FAFB"
  muted-steel: "#6F8289"
  signal-cyan: "#2BE5D2"
  hot-coral: "#F25F5C"
  ultraviolet: "#C99BFF"
  cue-yellow: "#FFCB47"
  connected-green: "#4DF7A4"
typography:
  display:
    fontFamily: "Barlow Condensed, sans-serif"
    fontSize: "30px"
    fontWeight: 700
    lineHeight: 1
    letterSpacing: "normal"
  status:
    fontFamily: "Atkinson Hyperlegible, sans-serif"
    fontSize: "11px"
    fontWeight: 700
    lineHeight: 1.1
    letterSpacing: "0.08em"
rounded:
  deck: "10px"
  indicator: "3px"
spacing:
  xs: "4px"
  sm: "8px"
  md: "16px"
components:
  preset-inactive:
    backgroundColor: "{colors.deck-surface}"
    textColor: "{colors.deck-white}"
    typography: "{typography.display}"
    rounded: "{rounded.deck}"
    padding: "16px"
  preset-active:
    backgroundColor: "{colors.hot-coral}"
    textColor: "{colors.stage-black}"
    typography: "{typography.display}"
    rounded: "{rounded.deck}"
    padding: "16px"
---

# Design System: MIDI Pedal — Chromatic Deck

## Overview

**Creative North Star: "Chromatic Deck"**

The display behaves like a compact stage instrument: four stable performance surfaces mirror the four physical switches, while preset color turns the entire screen into a memorable map. The interface is energetic but never ornamental; every large color field communicates identity or state.

The operating scene is a musician standing above a pedalboard under inconsistent stage light. Large condensed names, fixed positions, and redundant active-state cues take priority over density.

**Key Characteristics:**

- Saturated color fields anchored by a near-black stage surface.
- Stable 2×2 A–D spatial mapping.
- Condensed display type paired with unmistakable status text.
- Flat tonal layers with no shadows or simulated glass.

## Colors

The palette combines deep blue-black structural surfaces with user-configurable preset colors. Cyan is the system accent; coral, ultraviolet, and yellow demonstrate the intended chromatic range.

**The Full-Field State Rule.** Inactive presets use a narrow color rail; position 2 fills the complete tile and adds explicit state text.

**The Functional Color Rule.** Every color field carries preset identity, toggle state, connection status, or diagnostics. Decoration alone does not earn color.

## Typography

**Display Font:** Barlow Condensed Bold, converted to a flash-resident glyph atlas.

**Status Font:** Atkinson Hyperlegible Bold, converted to a flash-resident glyph atlas.

Large preset names are compact and assertive. Small status labels prioritize character distinction and remain readable from off-axis viewing angles.

**The Distance Test.** Switch letter, preset name, and active state must remain identifiable before bank/page details become readable.

## Layout

The live display uses a 480×320 fixed canvas: a compact status header, a 2×2 deck aligned with switches A–D, and a persistent expression footer. The four preset regions never reorder. Diagnostics temporarily replace the right side of the status header rather than shifting the deck.

## Elevation & Depth

The system is entirely flat. Depth comes from large changes in surface tone and state fill, not shadows, gradients, or transparency.

## Shapes

Preset surfaces use a confident 10 px radius. Color rails and expression tracks use small 3 px radii. Text and indicators align to an 8 px spacing rhythm.

## Components

### Preset Tile

Inactive tiles use the deck surface, a six-pixel preset-color rail, white name, muted state label, and colored switch letter. Active tiles become a full preset-color field with automatically selected light or dark foreground and an explicit `ON` label.

### Status Header

Bank number is subordinate to bank name. Page and USB state occupy the right edge. A connected dot accompanies text so connection is never color-only.

### Expression Footer

The assignment name and numeric value bracket one long horizontal meter. The meter uses the system cyan rather than the selected preset color.

## Do's and Don'ts

### Do:

- **Do** keep A, B, C, and D anchored to their physical switch positions.
- **Do** make active state legible through fill, wording, and contrast together.
- **Do** clip or abbreviate labels predictably instead of shrinking them below the distance threshold.

### Don't:

- **Don't** add shadows, glass effects, decorative gradients, or faux hardware texture.
- **Don't** animate continuously or make MIDI handling wait on display effects.
- **Don't** use color as the only indication of toggle, USB, warning, or error state.

---

## Browser Editor: Studio Console

The browser editor is the desk-side companion to Chromatic Deck. Its dark theme is a faithful studio-console expression of the live surface: a full-width command bar, persistent bank rail, dominant 2×2 switch deck, and tall inspector rendered in the same saturated functional palette.

### Editor composition

- The command bar spans the complete app frame: product lockup at left, device and file actions at right.
- The bank rail begins below the command bar and keeps search plus the numbered bank list visible.
- The A–D map is the visual center of gravity. Each switch becomes a large near-square field with a vivid cyan, coral, violet, or yellow identity, oversized letter, and compact label.
- Preset settings, ordered messages, and expression settings live in one right-hand inspector with clear section boundaries.
- At narrower widths the inspector moves below the map; on phones all regions become one readable vertical flow.

### Editor material and type

- The dark editor uses Stage Black, Signal Cyan, Hot Coral, Ultraviolet, Cue Yellow, and Connected Green directly. Light mode remains available as an accessible alternative.
- Saved preset colors remain authoritative. Empty factory presets use the four canonical A–D colors so the editor still communicates the physical layout before configuration.
- Use a neutral modern sans-serif stack with compact weights and tabular numbers. Headings are clear rather than oversized.
- Surfaces use restrained 10–14 px radii, crisp one-pixel borders, and shallow shadows only where they separate the application frame from the canvas.
- Controls are 38–42 px tall, labels remain visible, and focus rings are never removed.

### Editor interaction rules

- Selected states combine outline, surface change, and explicit text or `aria-current`.
- Connection, validation, and sync status are persistent and never color-only.
- Hover and press motion is brief and limited to direct manipulation; reduced-motion preferences remove it.
- File, device, and editing behavior must remain available at every supported breakpoint.
