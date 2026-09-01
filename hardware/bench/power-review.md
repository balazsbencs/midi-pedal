# Protected power bench review sheet

The KiCad sheet is a project placeholder until the selected regulator,
reverse-polarity stage, fuse, TVS, and source-isolation parts are measured and
entered. The intended topology is:

```text
USB VBUS ── reverse-current isolation ─┐
                                       ├── VSYS ── Pico 2
9 V center-negative ─ fuse ─ TVS ─ buck ┘
```

Required test points: raw 9 V, protected input, regulated 5 V, USB VBUS, VSYS,
3V3, and GND. The assembled circuit must not backfeed USB or the DC jack,
must survive source handover, and must be tested with a current-limited supply
before a Pico is fitted. Part selection and ERC are pending independent review.

