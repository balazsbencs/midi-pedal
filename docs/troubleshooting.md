# Troubleshooting

## No power or unexpectedly high current

Disconnect USB and 9 V immediately. With the Pico removed, verify the DC jack
is center-negative, inspect the fuse/TVS/reverse-polarity stage, and use a
current-limited supply. Do not bypass protection. Compare measured rails with
`hardware/validation/power-bench.md`; stop if any rail exceeds its documented
range.

## No USB interfaces or WebSerial says unavailable

Use a data-capable cable, reflash the UF2 through BOOTSEL, and try current
desktop Chrome/Edge/Brave on HTTPS (or localhost). A permission denial is
recoverable from **Connect pedal**; a busy port means close other serial
monitors. Safari, Firefox, mobile browsers, and insecure remote HTTP are not
supported.

## No TRS MIDI or USB-MIDI output

Confirm the selected preset has a PC/CC action, the channel/value are in range,
and the destination includes the output being monitored. Test one output at a
time, use a known Type-A cable, and capture at 31,250 baud. USB-MIDI can be
absent when no host is mounted; TRS output must remain independent. Power off
before changing jack wiring.

## Noisy, inverted, or absent expression

Use a voltage-divider TRS pedal in the 10–25 kΩ target range. Verify ring is
protected 3V3, tip is the wiper, and sleeve is ground. Recalibrate heel/toe,
check the minimum span (at least 410 ADC counts), and use the inversion setting
for mechanical direction. Unplugged or invalid input intentionally emits no
MIDI.

## Relay wrong or closed by default

Power off and use continuity mode. Both contacts must be open with no power,
during reset/BOOTSEL, and after a watchdog/brownout test. Verify relay TRS tip
and ring identity and never connect external amplifier voltages until an
isolation review approves them.

## Blank or wrong-color display

Power off and check the exact header order
`GND,VCC,SCL,SDA,RST,DC,CS,BL,SDA-0`. Logic is 3.3 V, BL is always on, and
SDA-0/SDO is intentionally unconnected. Confirm the module's VCC rating before
using the carrier supply selector. Compare the renderer golden state before
suspecting configuration.

## Upload failed or schema incompatible

Do not unplug repeatedly while powered. The atomic protocol leaves the prior
validated image active after a bad chunk, verify failure, or disconnect. Export
the local JSON, reconnect, check protocol/schema compatibility, and retry. Use
Factory empty reset only after making a backup.

## USB/9 V source handover resets the pedal

Disconnect both sources, stop using the board, and inspect the isolation stage.
Record which source was removed, rail measurements, current, and revision in
`hardware/validation/power-bench.md`. This is a hardware safety issue, not an
editor setting.

