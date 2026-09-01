# Power bench validation

Status: **pending physical bench work**. The table is the required evidence
record; blank values are not passes.

| Mode | VSYS idle | VSYS full load | USB reverse current | Reset observed | Evidence |
|---|---:|---:|---:|---|---|
| USB only | | | | | |
| 9 V only | | | | | |
| USB + 9 V | | | | | |
| Remove USB while 9 V remains | | | | | |
| Remove 9 V while USB remains | | | | | |

Procedure: current-limit the supply to 50 mA, fit no Pico for polarity/short
tests, sweep the protected input to 12 V, verify reverse 9 V is blocked, then
fit the Pico and repeat each handover. Record temperatures after 30 minutes.
Acceptance is VSYS within Pico limits, no reset on handover, no measurable
backfeed at 1 mA resolution, and no component over 30 °C rise.

