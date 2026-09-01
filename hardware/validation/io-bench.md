# I/O bench validation

Status: **pending physical bench work**. Test the actual modules and the
firmware commit recorded beside each capture.

| Interface | Required evidence | Status |
|---|---|---|
| MIDI OUT | 31,250 baud bytes for PC and CC, Type-A polarity/current-loop capture | Pending |
| Relay 1/2 | continuity OPEN/CLOSE/TOGGLE, isolation, reset/BOOTSEL/brownout/power-loss | Pending |
| Expression | 10 kΩ and 25 kΩ sweeps, unplug/replug, ADC/noise CSV | Pending |
| Switches | pull-up level, bounce, all chord timing | Pending |
| ST7796S | exact 9-pin order, 3.3 V logic, VCC option only when rated | Pending |

Attach instrument model, range, date, board revision, and raw capture path;
do not summarize an unmeasured row as a pass.

