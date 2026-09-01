# Hardware assembly (reference build)

This guide is for the measured reference carrier and is intentionally marked
WIP. Do not drill an enclosure or order a PCB from the current placeholder
outline. Complete the mechanical measurement CSV, schematic review, ERC/DRC,
and first-article checklist first.

## Parts and orientation

- Socket a non-wireless Raspberry Pi Pico 2 so its USB connector remains
  accessible through the enclosure.
- Mount the ST7796S above the four normally-open footswitches in landscape
  orientation. Header order is `GND,VCC,SCL,SDA,RST,DC,CS,BL,SDA-0`; BL is
  always on and SDA-0/SDO is not connected.
- Wire switches A/B top row and C/D bottom row. Use the firmware pin map rather
  than assuming connector order.
- Use one 3.5 mm TRS Type-A MIDI OUT jack and one 3.5 mm TRS relay jack. Relay
  tip/contact 1 and ring/contact 2 switch independently to sleeve.
- Use one 6.35 mm TRS expression jack for voltage-divider pedals only. Ring is
  protected 3V3, tip is the ADC wiper, and sleeve is ground.
- Use a center-negative 9 V DC jack. Confirm polarity with a meter before
  plugging in; USB and 9 V source handover must not backfeed either source.

## Assembly order

1. Measure every panel part and update `hardware/mechanical/measured-components.csv`.
2. Print `hardware/mechanical/drill-template.svg` at 100% and test a paper/acrylic
   mock-up with real plugs, washers, nuts, and shoe clearance.
3. Populate and inspect the carrier unpowered; verify relay contacts are open,
   protection orientation, display VCC selection, and connector labels.
4. Fit the socketed Pico, display, jacks, and switches. Add strain relief and
   keep all unsleeved conductors away from the aluminum enclosure.
5. Power from a current-limited bench supply in the sequence in
   `hardware/validation/power-bench.md`, then run the firmware checks.

## Disassembly and safety

Disconnect USB and 9 V before opening the enclosure. Never connect a relay
contact to an amplifier switching voltage until an independent isolation review
has approved that use. Relay outputs are dry contacts, not logic outputs. Do
not change the display supply selector while powered.

