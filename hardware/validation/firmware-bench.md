# Firmware bench validation

This log is intentionally a checklist until a physical Pico 2 and carrier are
available. Do not treat an unchecked row as a hardware pass.

| Check | Instrument/procedure | Evidence | Status |
|---|---|---|---|
| TRS MIDI timing | Logic analyzer on MIDI DIN output; 31,250 baud, 8-N-1; capture a press and compare the first byte timestamp | Attach capture and measured latency | Pending hardware |
| Relay 1/2 fail-open | Continuity meter while powering, resetting, holding BOOTSEL, watchdog resetting, and unplugging | Attach continuity notes/photos | Pending hardware |
| Expression ADC | Calibrated pedal at heel, toe, and unplugged; record ADC counts and emitted CC values | Attach CSV capture | Pending hardware |
| USB MIDI | Host enumerates CDC and MIDI interfaces; monitor one CC packet | Attach host/device capture | Pending hardware |

The native contract tests and the Pico 2 release build are automated checks;
they do not replace these electrical measurements. Relay contacts are designed
as normally open, and firmware also drives both outputs low before configuration
is read.

