# Display validation

The renderer geometry and RGB565 output are covered by native tests and the
committed golden rasters in `firmware/tests/golden/display/`. Physical timing
and panel readability still require a connected ST7796S module.

| Check | Procedure | Evidence | Status |
|---|---|---|---|
| Initialization | Flash the Pico 2, observe reset/sleep-out/display-on sequence, and confirm 480×320 landscape orientation | Photo and logic-analyzer trace | Pending hardware |
| Dirty update | Capture a switch press and measure first visible pixel change | Timestamped capture; target under 100 ms | Pending hardware |
| Full screen | Capture factory-empty, toggle position 1/2, unavailable expression, and recoverable-error states | Photos linked to build/revision | Pending hardware |
| Backlight/SDO wiring | Confirm backlight is hardwired on and SDO is intentionally not connected | Wiring photo | Pending hardware |

