---
version: 1
slug: "firmware-src-display-live-renderer-cpp"
primary_target: "firmware/src/display/live_renderer.cpp"
related_targets: ["firmware/src/display/live_renderer.hpp"]
---

# Live display surface brief

- Mode: operate. A musician standing above the pedalboard must identify bank, four switch assignments, active toggle states, and expression status at a glance.
- Direction: Chromatic Deck. The stable 2×2 deck mirrors A–D; inactive presets carry a narrow identity rail, while position 2 becomes a full color field with explicit `ON` wording.
- Memorable moment: pressing a toggle turns its entire tile into the preset color without moving any content.
- Constraints: 480×320 ST7796S, RGB565, Pico 2, four switches, no touch, deterministic MIDI path, flash-resident fonts, and region-based redraws.
- Scope: live screen only. Configuration remains in the browser editor. Continuous animation, decorative imagery, and a generalized widget framework are out of scope.
