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

## Firmware update performance

The current panel workaround requires a full 480×320 address window at the
hardware-tested 8 MHz SPI request. Partial vertical windows are unreliable on
this module. Each refresh therefore still sends 307,200 bytes; its ideal wire
time is approximately 307 ms, before command and scheduling overhead. The
original PR's 93–96% SPI traffic reduction does not apply to this workaround.

DMA sends four scanlines per chunk while the control loop continues processing
switches, expression, MIDI, USB, and watchdog work. `service()` returns while
DMA or SPI is busy; it retains CS until the final bit of the complete frame has
left the wire. A 3,840-byte staging buffer keeps queued pixels stable when
`present()` updates the framebuffer during a transfer. Updates received during
transmission remain pending for a subsequent full refresh.

The renderer compares old/new RGB565 tiles and copies only changed bounds into
the framebuffer. A label-only `RHYTHM` → `LEAD` change copies 2,470 pixels instead
of the 23,940 pixels in a complete deck tile (89.7% fewer copied pixels). This
is a framebuffer-copy measurement, not a wire-traffic or visible-latency claim.
Press and toggle changes intentionally fill the deck tile and require larger
updates. The new deck geometry, fonts, colors, press feedback, and committed
golden rasters are preserved.

The renderer uses three 4 KiB scratch tiles (8 KiB more than the original
renderer). Together with the DMA staging buffer this adds 11.75 KiB of bounded
RAM to the framebuffer-based implementation.

Native tests compare incremental output with fresh renders across 80 state
transitions, including press/release, shorter labels, expression changes,
diagnostics, accent changes, and invalidation. Device-branch SDK-fake tests
verify non-blocking DMA service, staging-buffer lifetime, final SPI drain,
complete frame transfers, subsequent refreshes for intervening updates, and
rejection of incomplete/out-of-bounds writes.

On hardware, measure accepted-event-to-first-pixel and accepted-event-to-last-pixel
latency while expression and USB MIDI are active; check TRS/USB MIDI latency as
well. Attach traces/photos above before claiming physical timing qualification.
