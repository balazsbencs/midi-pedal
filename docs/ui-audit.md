# Editor UI audit

The editor follows the approved three-pane hierarchy and adds the explicitly
requested Light/Dark switch. This audit records the checks that can be run in
the repository and separates them from hardware/browser evidence that needs a
real device.

## Reviewed decisions

- The physical A/B/C/D order is preserved in a 2×2 map and in DOM reading order.
- Banks, pages, presets, position toggles, message controls, and theme changes
  are real keyboard-operable buttons/inputs with visible focus.
- Status is communicated with text as well as color; validation failures name
  the path and keep Sync disabled.
- At narrower widths the inspector moves below the bank/map columns; at 200%
  zoom the layout can flow vertically without a required horizontal scroll.
- Light and dark palettes use semantic tokens, with `prefers-color-scheme`
  used only for the first visit and explicit choices stored locally.

## Automated evidence

Run:

```bash
pnpm --filter @midi-pedal/editor typecheck
pnpm --filter @midi-pedal/editor test
pnpm --filter @midi-pedal/editor test:e2e
```

The component tests cover theme precedence, keyboard selection, stable
workspace order, dirty draft editing, and simulated transport failures. The
Chromium tests cover labelled controls, theme keyboard reachability, desktop
viewports, narrow layout, and a 200% zoom overflow check.

## Remaining manual evidence

The audit does not claim physical-device success. A release still needs a
current stable Chrome/Edge/Brave pass against a flashed controller, plus
contrast confirmation in the exact hosted font/rendering environment. Record
browser, OS, firmware, and commit in the real-device checklist.

