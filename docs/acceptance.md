# Product acceptance evidence

This table is the release map from the design specification to evidence. A
`PENDING` or `BLOCKED` row with no linked capture keeps a release from being
called qualified.

| Requirement | HIL/manual case | Instrument or automation | Limit/status | Evidence |
|---|---|---|---|---|
| Press-to-MIDI latency | switch.latency-under-50ms | logic analyzer + HIL timestamp | <50 ms | Pending hardware |
| Four approved chords/repeat | switch.chord-* | switch fixture + firmware trace | exact map/timing | Native tests; hardware pending |
| TRS/USB bytes and routing | midi.trs-bytes, midi.usb-packets, midi.per-message-routing | MIDI monitor/logic analyzer | exact bytes/order | Native encoder tests; hardware pending |
| Queue order/overflow | midi.queue-order | deterministic queue test | drop newest only | Native tests |
| Relay open boot/reset/watchdog/brownout/unpowered | relay.* | DMM/continuity logger | contacts open | Pending hardware |
| Expression filtering/calibration | expression.calibration-sweep | 10/25 kΩ sweep + ADC capture | no MIDI when invalid/disconnected | Native tests; hardware pending |
| Display feedback | display.response-under-100ms | panel capture/photograph | visible <100 ms | Renderer goldens; hardware pending |
| USB-only/9 V-only/handover | power.* | current-limited supply + DMM | no reset/backfeed | Pending hardware |
| Interrupted upload recovery | config.interrupted-upload | power-cut fixture + CRC readback | old image remains active | Native ConfigStore tests; hardware pending |
| Editor keyboard/themes/200% | editor.* | Chromium + Playwright | WCAG AA/no required horizontal scroll | Chromium tests |

The current `pnpm hil:release -- --rig simulated` report intentionally records
hardware-dependent rows as release-blocking skips. Attach a real report to a
tagged release only after the first-article and bench records are complete.

