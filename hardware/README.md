# Carrier hardware (work in progress)

The reference build uses a socketed Raspberry Pi Pico 2 on a two-layer carrier
inside a measured Hammond 1590DD-style enclosure. The electrical intent is
captured in the approved design specification and the bench-review sheets in
this directory. Production footprints and fabrication exports must not be
released until they have been checked against the actual display, jacks,
switches, relays, and enclosure.

Current repository artifacts:

- `midi-pedal.kicad_pcb` is a safe board-outline/project starting point; it is
  not a routed production board.
- `bench/` contains review sheets for protected power and I/O circuits.
- `mechanical/measured-components.csv` records the measurements still needed
  before cutouts are frozen.
- `validation/` contains evidence tables; unchecked hardware rows are pending.

Never connect an unverified relay contact or external amplifier voltage to a
Pico GPIO. The two relay contacts must remain dry, isolated, and normally open.
Display logic is 3.3 V; verify a breakout's VCC rating before selecting its
3.3 V/5 V supply jumper. The ST7796S backlight is always on and SDO is not
routed.

